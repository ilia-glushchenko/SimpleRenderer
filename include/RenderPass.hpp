/*
 * Copyright (C) 2019 by Ilya Glushchenko
 * This code is licensed under the MIT license (MIT)
 * (http://opensource.org/licenses/MIT)
 */
#pragma once

#include <RendererDefinitions.hpp>

void DeleteRenderPass(RenderPass &pass)
{
    DeleteShaderProgram(pass.program);
    glDeleteFramebuffers(RenderPass::MAX_FBOS, pass.fbos);
    for (uint32_t i = 0; i < RenderPass::MAX_TEXTURES; ++i)
    {
        glDeleteTextures(2, pass.textures);
    }
}

void UpdateGlobalUniforms(ShaderProgram const &program)
{
    for (auto &uniform : program.unifromsui32)
    {
        glUniform1ui(uniform.location, *uniform.data);
    }
    for (auto &uniform : program.uniformsf)
    {
        glUniform1f(uniform.location, *uniform.data);
    }
    for (auto &uniform : program.uniforms3f)
    {
        glUniform3fv(uniform.location, 1, uniform.data);
    }
    for (auto &uniform : program.uniforms16f)
    {
        glUniformMatrix4fv(uniform.location, 1, GL_TRUE, uniform.data);
    }
}

uint32_t BindDrawModelDependencies(RenderModel const &model)
{
    uint32_t bound = 0;

    if (model.albedoTexture != 0)
    {
        bound++;
        glActiveTexture(GL_TEXTURE0 + 1);
        glBindTexture(GL_TEXTURE_2D, model.albedoTexture);
    }
    if (model.normalTexture != 0)
    {
        bound++;
        glActiveTexture(GL_TEXTURE0 + 2);
        glBindTexture(GL_TEXTURE_2D, model.normalTexture);
    }
    if (model.bumpTexture != 0)
    {
        bound++;
        glActiveTexture(GL_TEXTURE0 + 3);
        glBindTexture(GL_TEXTURE_2D, model.bumpTexture);
    }
    if (model.metallicTexture != 0)
    {
        bound++;
        glActiveTexture(GL_TEXTURE0 + 4);
        glBindTexture(GL_TEXTURE_2D, model.metallicTexture);
    }
    if (model.roughnessTexture != 0)
    {
        bound++;
        glActiveTexture(GL_TEXTURE0 + 5);
        glBindTexture(GL_TEXTURE_2D, model.roughnessTexture);
    }

    return bound;
}

void BindRenderPassDependencies(GLuint const (&dependencies)[RenderPass::MAX_DEPENDENCIES], uint32_t count)
{
    for (uint32_t i = 0; i < count; ++i)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, dependencies[i]);
    }
}

void DrawModel(RenderModel const &model)
{
    glBindVertexArray(model.vertexArrayObject);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.indexBuffer);
    glDrawElements(GL_TRIANGLES, model.indexCount, GL_UNSIGNED_INT, nullptr);
}

void ExecuteRenderPass(RenderPass const &pass, RenderModel const *models, uint32_t modelCount)
{
    glBindFramebuffer(GL_FRAMEBUFFER, pass.fbo1);
    {
        glUseProgram(pass.program.handle);

        glViewport(0, 0, pass.width, pass.height);
        glScissor(0, 0, pass.width, pass.height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        UpdateGlobalUniforms(pass.program);
        BindRenderPassDependencies(pass.dependencies, pass.dependencyCount);

        for (uint32_t i = 0; i < modelCount; ++i)
        {
            DrawModel(models[i]);
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ExectureBackBufferBlitRenderPass(RenderPass const &lastPass, GLenum attachment)
{
    glBindFramebuffer(GL_READ_FRAMEBUFFER, lastPass.fbo1);
    {
        glReadBuffer(attachment);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

        glViewport(0, 0, lastPass.width, lastPass.height);
        glScissor(0, 0, lastPass.width, lastPass.height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBlitFramebuffer(
            0, 0, lastPass.width, lastPass.height,
            0, 0, lastPass.width, lastPass.height,
            GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
