/*
 * Copyright (C) 2019 by Ilya Glushchenko
 * This code is licensed under the MIT license (MIT)
 * (http://opensource.org/licenses/MIT)
 */
#pragma once

#include "RenderDefinitions.hpp"
#include "ShaderProgram.hpp"

#include <cassert>

RenderPass CreateRenderPass(
    SubPassDescriptor const *desc, uint8_t count,
    ShaderProgram program,
    int32_t width, int32_t height,
    char const(name)[SHORT_STRING_MAX_LENGTH], uint8_t length)
{
    assert(desc != nullptr);
    assert(count < RENDER_PASS_MAX_SUBPASS);
    assert(length <= SHORT_STRING_MAX_LENGTH);

    RenderPass pass;
    std::memcpy(pass.name.data, name, sizeof(char) * length);
    pass.name.length = length;
    pass.program = program;
    pass.subPassCount = count;
    pass.width = width;
    pass.height = height;

    GLuint fbos[RENDER_PASS_MAX_SUBPASS] = {};
    glGenFramebuffers(pass.subPassCount, fbos);

    for (uint8_t i = 0; i < pass.subPassCount; ++i)
    {
        SubPass &subPass = pass.subPasses[i];
        subPass.fbo = fbos[i];
        subPass.active = true;
        subPass.desc = desc[i];

        glBindFramebuffer(GL_FRAMEBUFFER, subPass.fbo);
        {
            GLenum drawBuffers[RENDER_PASS_MAX_ATTACHMENTS] = {};
            uint8_t drawBufferCount = 0;

            for (uint8_t j = 0; j < subPass.desc.attachmentCount; ++j)
            {
                auto const &attachment = subPass.desc.attachments[j];

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(attachment.texture, attachment.handle);
                glFramebufferTexture2D(GL_FRAMEBUFFER, attachment.attachment, attachment.texture, attachment.handle, 0);

                if (attachment.attachment >= GL_COLOR_ATTACHMENT0 && attachment.attachment <= GL_COLOR_ATTACHMENT31)
                {
                    drawBuffers[drawBufferCount++] = attachment.attachment;
                }
            }

            glDrawBuffers(drawBufferCount, drawBuffers);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            {
                std::cerr << "Error! Failed to create SubPass! FrameBuffer is not complete!" << std::endl;
            }
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    return pass;
}

void DeleteRenderPass(RenderPass &pass)
{
    glDeleteProgram(pass.program.handle);
    glDeleteShader(pass.program.vertexShaderHandle);
    glDeleteShader(pass.program.fragmentShaderHandle);

    for (uint32_t i = 0; i < pass.subPassCount; ++i)
    {
        // Free attachments
        for (uint32_t j = 0; j < pass.subPasses[i].desc.attachmentCount; ++j)
        {
            glDeleteTextures(1, &pass.subPasses[i].desc.attachments[j].handle);
        }

        glDeleteFramebuffers(1, &pass.subPasses[i].fbo);
    }
}

void UpdatePerFrameUniforms(ShaderProgram const &program)
{
    for (uint32_t i = 0; i < program.perFrameUniformBindings.UI32.count; ++i)
    {
        auto const &uniform = program.perFrameUniformBindings.UI32.data[i];
        glUniform1uiv(uniform.location, uniform.count, uniform.data);
    }
    for (uint32_t i = 0; i < program.perFrameUniformBindings.Float1.count; ++i)
    {
        auto const &uniform = program.perFrameUniformBindings.Float1.data[i];
        glUniform1fv(uniform.location, uniform.count, uniform.data);
    }
    for (uint32_t i = 0; i < program.perFrameUniformBindings.Float2.count; ++i)
    {
        auto const &uniform = program.perFrameUniformBindings.Float2.data[i];
        glUniform2fv(uniform.location, uniform.count, uniform.data);
    }
    for (uint32_t i = 0; i < program.perFrameUniformBindings.Float3.count; ++i)
    {
        auto const &uniform = program.perFrameUniformBindings.Float3.data[i];
        glUniform3fv(uniform.location, uniform.count, uniform.data);
    }
    for (uint32_t i = 0; i < program.perFrameUniformBindings.Float4.count; ++i)
    {
        auto const &uniform = program.perFrameUniformBindings.Float4.data[i];
        glUniform4fv(uniform.location, uniform.count, uniform.data);
    }
    for (uint32_t i = 0; i < program.perFrameUniformBindings.Float16.count; ++i)
    {
        auto const &uniform = program.perFrameUniformBindings.Float16.data[i];
        glUniformMatrix4fv(uniform.location, uniform.count, GL_TRUE, uniform.data);
    }
}

void UpdatePerModelUniforms(ShaderProgram const &program, uint64_t index)
{
    for (uint32_t i = 0; i < program.perModelUniformBindings.UI32.count; ++i)
    {
        auto const &uniform = program.perModelUniformBindings.UI32.data[i];
        uint8_t const *data = reinterpret_cast<uint8_t const *>(uniform.data);
        data = data + uniform.offset + uniform.stride * index;
        glUniform1ui(uniform.location, *reinterpret_cast<uint32_t const *>(data));
    }
    for (uint32_t i = 0; i < program.perModelUniformBindings.Float1.count; ++i)
    {
        auto const &uniform = program.perModelUniformBindings.Float1.data[i];
        uint8_t const *data = reinterpret_cast<uint8_t const *>(uniform.data);
        data = data + uniform.offset + uniform.stride * index;
        glUniform1f(uniform.location, *reinterpret_cast<float const *>(data));
    }
    for (uint32_t i = 0; i < program.perModelUniformBindings.Float2.count; ++i)
    {
        auto const &uniform = program.perModelUniformBindings.Float2.data[i];
        uint8_t const *data = reinterpret_cast<uint8_t const *>(uniform.data);
        data = data + uniform.offset + uniform.stride * index;
        glUniform2fv(uniform.location, 1, reinterpret_cast<float const *>(data));
    }
    for (uint32_t i = 0; i < program.perModelUniformBindings.Float3.count; ++i)
    {
        auto const &uniform = program.perModelUniformBindings.Float3.data[i];
        uint8_t const *data = reinterpret_cast<uint8_t const *>(uniform.data);
        data = data + uniform.offset + uniform.stride * index;
        glUniform3fv(uniform.location, 1, reinterpret_cast<float const *>(data));
    }
    for (uint32_t i = 0; i < program.perModelUniformBindings.Float4.count; ++i)
    {
        auto const &uniform = program.perModelUniformBindings.Float4.data[i];
        uint8_t const *data = reinterpret_cast<uint8_t const *>(uniform.data);
        data = data + uniform.offset + uniform.stride * index;
        glUniform4fv(uniform.location, 1, reinterpret_cast<float const *>(data));
    }
    for (uint32_t i = 0; i < program.perModelUniformBindings.Float16.count; ++i)
    {
        auto const &uniform = program.perModelUniformBindings.Float16.data[i];
        uint8_t const *data = reinterpret_cast<uint8_t const *>(uniform.data);
        data = data + uniform.offset + uniform.stride * index;
        glUniformMatrix4fv(uniform.location, 1, GL_TRUE, reinterpret_cast<float const *>(data));
    }
}

void BindRenderPassDependencies(GLuint const (&dependencies)[RENDER_PASS_MAX_DEPENDENCIES], uint32_t count)
{
    for (uint32_t i = 0; i < count; ++i)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, dependencies[i]);
    }
}

void BindRenderPassDependencies(SubPassDependencyDescriptor const (&dependencies)[RENDER_PASS_MAX_DEPENDENCIES], uint8_t count)
{
    for (uint8_t i = 0; i < count; ++i)
    {
        glActiveTexture(dependencies[i].unit);
        glBindTexture(dependencies[i].texture, dependencies[i].handle);
    }
}

void UnbindRenderPassDependencies(SubPassDependencyDescriptor const (&dependencies)[RENDER_PASS_MAX_DEPENDENCIES], uint8_t count)
{
    for (uint8_t i = 0; i < count; ++i)
    {
        glActiveTexture(dependencies[i].unit);
        glBindTexture(dependencies[i].texture, 0);
    }
}

void BindRenderModelTextures(RenderModel const &model, uint32_t bindingOffset)
{
    if (model.albedoTexture != 0)
    {
        glActiveTexture(GL_TEXTURE0 + bindingOffset + 0);
        glBindTexture(GL_TEXTURE_2D, model.albedoTexture);
    }
    if (model.normalTexture != 0)
    {
        glActiveTexture(GL_TEXTURE0 + bindingOffset + 1);
        glBindTexture(GL_TEXTURE_2D, model.normalTexture);
    }
    if (model.bumpTexture != 0)
    {
        glActiveTexture(GL_TEXTURE0 + bindingOffset + 2);
        glBindTexture(GL_TEXTURE_2D, model.bumpTexture);
    }
    if (model.metallicTexture != 0)
    {
        glActiveTexture(GL_TEXTURE0 + bindingOffset + 3);
        glBindTexture(GL_TEXTURE_2D, model.metallicTexture);
    }
    if (model.roughnessTexture != 0)
    {
        glActiveTexture(GL_TEXTURE0 + bindingOffset + 4);
        glBindTexture(GL_TEXTURE_2D, model.roughnessTexture);
    }
}

void UnbindRenderModelTextures(RenderModel const &model, uint32_t bindingOffset)
{
    if (model.albedoTexture != 0)
    {
        glActiveTexture(GL_TEXTURE0 + bindingOffset + 0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    if (model.normalTexture != 0)
    {
        glActiveTexture(GL_TEXTURE0 + bindingOffset + 1);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    if (model.bumpTexture != 0)
    {
        glActiveTexture(GL_TEXTURE0 + bindingOffset + 2);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    if (model.metallicTexture != 0)
    {
        glActiveTexture(GL_TEXTURE0 + bindingOffset + 3);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    if (model.roughnessTexture != 0)
    {
        glActiveTexture(GL_TEXTURE0 + bindingOffset + 4);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void DrawModel(RenderModel const &model)
{
    glBindVertexArray(model.vertexArrayObject);
    glDrawElements(GL_TRIANGLES, model.indexCount, GL_UNSIGNED_INT, nullptr);
}

void ExecuteRenderPass(RenderPass const &pass, RenderModel const *models, uint64_t modelCount)
{
#ifdef NDEBUG
    glPushGroupMarkerEXT(pass.name.length, pass.name.data);
#endif

    for (uint8_t i = 0; i < pass.subPassCount; ++i)
    {
        auto &subPass = pass.subPasses[i];
        if (subPass.active)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, subPass.fbo);
            {
                glClearColor(static_cast<float>(subPass.desc.colorClearValue[0]),
                             static_cast<float>(subPass.desc.colorClearValue[1]),
                             static_cast<float>(subPass.desc.colorClearValue[2]),
                             static_cast<float>(subPass.desc.colorClearValue[3]));
                glColorMask(subPass.desc.enableClearColorBuffer,
                            subPass.desc.enableClearColorBuffer,
                            subPass.desc.enableClearColorBuffer,
                            subPass.desc.enableClearColorBuffer);

                glClearDepth(static_cast<float>(subPass.desc.depthClearValue));
                glDepthMask(subPass.desc.enableWriteToDepth);
                glDepthFunc(subPass.desc.depthTestFunction);

                glViewport(0, 0, pass.width, pass.height);
                glScissor(0, 0, pass.width, pass.height);

                glClear(
                    (subPass.desc.enableClearColorBuffer ? ClearBufferMask::GL_COLOR_BUFFER_BIT : ClearBufferMask::GL_NONE_BIT) | (subPass.desc.enableClearDepthBuffer ? ClearBufferMask::GL_DEPTH_BUFFER_BIT : ClearBufferMask::GL_NONE_BIT));

                glUseProgram(pass.program.handle);
                UpdatePerFrameUniforms(pass.program);
                BindRenderPassDependencies(subPass.desc.dependencies, subPass.desc.dependencyCount);

                for (uint64_t j = 0; j < modelCount; ++j)
                {
                    UpdatePerModelUniforms(pass.program, j);
                    BindRenderModelTextures(models[j], subPass.desc.dependencyCount);
                    DrawModel(models[j]);
                    UnbindRenderModelTextures(models[j], subPass.desc.dependencyCount);
                }

                UnbindRenderPassDependencies(subPass.desc.dependencies, subPass.desc.dependencyCount);
                glUseProgram(0);
            }
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
    }

#ifdef NDEBUG
    glPopGroupMarkerEXT();
#endif
}

void ExecuteBackBufferBlitRenderPass(GLuint fbo, GLenum attachment, int32_t width, int32_t height)
{
#ifdef NDEBUG
    glPushGroupMarkerEXT(sizeof("Blit framebuffer"), "Blit framebuffer");
#endif

    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
    {
        glReadBuffer(attachment);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

        glViewport(0, 0, width, height);
        glScissor(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBlitFramebuffer(
            0, 0, width, height,
            0, 0, width, height,
            GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

#ifdef NDEBUG
    glPopGroupMarkerEXT();
#endif
}

SubPassDescriptor CreateDefaultSubPassDescriptor()
{
    SubPassDescriptor desc;

    desc.dependencyCount = 0;
    desc.attachmentCount = 0;

    desc.enableWriteToDepth = false;
    desc.enableClearDepthBuffer = false;
    desc.depthClearValue = 1;
    desc.depthTestFunction = GL_ALWAYS;

    desc.enableWriteToColor = false;
    desc.enableClearColorBuffer = false;
    memset(desc.colorClearValue, 0, sizeof(SubPassDescriptor::colorClearValue));

    desc.prePassCallback = nullptr;
    desc.postPassCallback = nullptr;

    return desc;
}
