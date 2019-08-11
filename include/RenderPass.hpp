/*
 * Copyright (C) 2019 by Ilya Glushchenko
 * This code is licensed under the MIT license (MIT)
 * (http://opensource.org/licenses/MIT)
 */
#pragma once

#include <RendererDefinitions.hpp>

RenderPass CreateRenderPass(SubPassDescriptor const *desc, uint8_t count, ShaderProgram program, int32_t width, int32_t height)
{
    assert(desc != nullptr);
    assert(count < RENDER_PASS_MAX_SUBPASS);

    RenderPass pass;
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
    DeleteShaderProgram(pass.program);
    for (uint8_t i = 0; i < pass.subPassCount; ++i)
    {
        glDeleteFramebuffers(1, &(pass.subPasses[i].fbo));
    }
}

Pipeline CreateRenderPipeline(PipelineShaderPrograms programs, int32_t width, int32_t height)
{
    Pipeline pipeline;

    {
        SubPassDescriptor desc;
        desc.attachmentCount = 1;
        desc.attachments[0] = SubPassAttachmentDescriptor{
            GL_DEPTH_ATTACHMENT,
            GL_TEXTURE_2D,
            CreateDepthTexture(4096, 4096)};

        pipeline.shadowMapping = CreateRenderPass(&desc, 1, programs.shadowMapping, 4096, 4096);
    }

    {
        SubPassDescriptor desc;
        desc.dependencyCount = 1;
        desc.dependencies[0] = SubPassDependencyDescriptor{
            GL_TEXTURE0,
            GL_TEXTURE_2D,
            pipeline.shadowMapping.subPasses[0].desc.attachments[0].handle};
        desc.attachmentCount = 2;
        desc.attachments[0] = SubPassAttachmentDescriptor{
            GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, CreateEmptyRGBATexture(width, height)};
        desc.attachments[1] = SubPassAttachmentDescriptor{
            GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, CreateDepthTexture(width, height)};

        pipeline.lighting = CreateRenderPass(&desc, 1, programs.lighting, width, height);
    }

    {
        SubPassDescriptor desc;

        desc.attachmentCount = 1;
        desc.attachments[0] = SubPassAttachmentDescriptor{
            GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, CreateEmptyRGBATexture(width, height)};

        pipeline.velocity = CreateRenderPass(&desc, 1, programs.velocity, width, height);
    }

    {
        auto const texture1 = CreateEmptyRGBATexture(width, height);
        auto const texture2 = CreateEmptyRGBATexture(width, height);

        SubPassDescriptor desc[2];

        desc[0].dependencyCount = 4;
        desc[0].dependencies[0] = SubPassDependencyDescriptor{
            GL_TEXTURE0, GL_TEXTURE_2D, pipeline.lighting.subPasses[0].desc.attachments[0].handle};
        desc[0].dependencies[1] = SubPassDependencyDescriptor{
            GL_TEXTURE1, GL_TEXTURE_2D, pipeline.lighting.subPasses[0].desc.attachments[1].handle};
        desc[0].dependencies[2] = SubPassDependencyDescriptor{
            GL_TEXTURE2, GL_TEXTURE_2D, texture2};
        desc[0].dependencies[3] = SubPassDependencyDescriptor{
            GL_TEXTURE3, GL_TEXTURE_2D, pipeline.velocity.subPasses[0].desc.attachments[0].handle};
        desc[0].attachmentCount = 1;
        desc[0].attachments[0] = SubPassAttachmentDescriptor{GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture1};

        desc[1].dependencyCount = desc[0].dependencyCount;
        desc[1].dependencies[0] = desc[0].dependencies[0];
        desc[1].dependencies[1] = desc[0].dependencies[1];
        desc[1].dependencies[2] = SubPassDependencyDescriptor{GL_TEXTURE2, GL_TEXTURE_2D, texture1};
        desc[1].dependencies[3] = desc[0].dependencies[3];
        desc[1].attachmentCount = 1;
        desc[1].attachments[0] = SubPassAttachmentDescriptor{GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture2};

        pipeline.taa = CreateRenderPass(desc, 2, programs.taa, width, height);
        pipeline.taa.subPasses[1].active = false;
    }

    {
        SubPassDescriptor desc;
        desc.dependencyCount = 5;
        desc.dependencies[0] = SubPassDependencyDescriptor{
            GL_TEXTURE0,
            GL_TEXTURE_2D,
            pipeline.lighting.subPasses[0].desc.attachments[0].handle};
        desc.dependencies[1] = SubPassDependencyDescriptor{
            GL_TEXTURE1,
            GL_TEXTURE_2D,
            pipeline.lighting.subPasses[0].desc.attachments[1].handle};
        desc.dependencies[2] = SubPassDependencyDescriptor{
            GL_TEXTURE2,
            GL_TEXTURE_2D,
            pipeline.taa.subPasses[1].desc.attachments[0].handle};
        desc.dependencies[3] = SubPassDependencyDescriptor{
            GL_TEXTURE3,
            GL_TEXTURE_2D,
            pipeline.taa.subPasses[0].desc.attachments[0].handle};
        desc.dependencies[4] = SubPassDependencyDescriptor{
            GL_TEXTURE4,
            GL_TEXTURE_2D,
            pipeline.velocity.subPasses[0].desc.attachments[0].handle};
        desc.attachmentCount = 1;
        desc.attachments[0] = SubPassAttachmentDescriptor{
            GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D,
            CreateEmptyRGBATexture(width, height)};

        pipeline.debug = CreateRenderPass(&desc, 1, programs.debug, width, height);
    }

    return pipeline;
}

void UpdateGlobalUniforms(ShaderProgram const &program)
{
    for (auto &uniform : program.ui32)
    {
        glUniform1ui(uniform.location, *uniform.data);
    }
    for (auto &uniform : program.f1)
    {
        glUniform1f(uniform.location, *uniform.data);
    }
    for (auto &uniform : program.f3)
    {
        glUniform3fv(uniform.location, 1, uniform.data);
    }
    for (auto &uniform : program.f16)
    {
        glUniformMatrix4fv(uniform.location, 1, GL_TRUE, uniform.data);
    }
}

void UpdateLocalUniforms(ShaderProgram const &program, uint64_t index)
{
    for (auto &uniform : program.ui32Array)
    {
        uint8_t const *data = reinterpret_cast<uint8_t const *>(uniform.data);
        data = data + uniform.offset + uniform.stride * index;
        glUniform1ui(uniform.location, *reinterpret_cast<uint32_t const *>(data));
    }
    for (auto &uniform : program.f1Array)
    {
        uint8_t const *data = reinterpret_cast<uint8_t const *>(uniform.data);
        data = data + uniform.offset + uniform.stride * index;
        glUniform1f(uniform.location, *reinterpret_cast<float const *>(data));
    }
    for (auto &uniform : program.f3Array)
    {
        uint8_t const *data = reinterpret_cast<uint8_t const *>(uniform.data);
        data = data + uniform.offset + uniform.stride * index;
        glUniform3fv(uniform.location, 1, reinterpret_cast<float const *>(data));
    }
    for (auto &uniform : program.f16Array)
    {
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
    for (uint8_t i = 0; i < pass.subPassCount; ++i)
    {
        if (pass.subPasses[i].active)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, pass.subPasses[i].fbo);
            {
                glUseProgram(pass.program.handle);

                glViewport(0, 0, pass.width, pass.height);
                glScissor(0, 0, pass.width, pass.height);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                UpdateGlobalUniforms(pass.program);
                BindRenderPassDependencies(pass.subPasses[i].desc.dependencies, pass.subPasses[i].desc.dependencyCount);

                for (uint64_t j = 0; j < modelCount; ++j)
                {
                    UpdateLocalUniforms(pass.program, j);
                    BindRenderModelTextures(models[j], pass.subPasses[i].desc.dependencyCount);
                    DrawModel(models[j]);
                    UnbindRenderModelTextures(models[j], pass.subPasses[i].desc.dependencyCount);
                }

                UnbindRenderPassDependencies(pass.subPasses[i].desc.dependencies, pass.subPasses[i].desc.dependencyCount);
            }
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
    }
}

void ExecuteBackBufferBlitRenderPass(GLuint fbo, GLenum attachment, int32_t width, int32_t height)
{
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
}
