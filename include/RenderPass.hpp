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

Pipeline CreateRenderPipeline(PipelineShaderPrograms programs, int32_t width, int32_t height)
{
    Pipeline pipeline;

    {
        SubPassDescriptor desc;
        desc.attachmentCount = 1;
        desc.attachments[0] = SubPassAttachmentDescriptor{
            GL_DEPTH_ATTACHMENT,
            GL_TEXTURE_2D,
            CreateDepthTexture(width, height)};
        desc.depthMask = GL_TRUE;
        desc.depthFunc = GL_LESS;
        desc.clearBufferMask = GL_DEPTH_BUFFER_BIT;

        auto const name = "Depth Pre-pass";
        pipeline.depthPrePass = CreateRenderPass(&desc, 1, programs.depthPrePass, width, height, name, std::strlen(name));
    }

    {
        SubPassDescriptor desc;
        desc.attachmentCount = 1;
        desc.attachments[0] = SubPassAttachmentDescriptor{
            GL_DEPTH_ATTACHMENT,
            GL_TEXTURE_2D,
            CreateDepthTexture(4096, 4096)};
        desc.depthMask = GL_TRUE;
        desc.depthFunc = GL_LESS;
        desc.clearBufferMask = GL_DEPTH_BUFFER_BIT;

        auto const name = "Shadow Mapping";
        pipeline.shadowMapping = CreateRenderPass(&desc, 1, programs.shadowMapping, 4096, 4096, name, std::strlen(name));
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
            GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, CreateColorAttachment(width, height)};
        desc.attachments[1] = SubPassAttachmentDescriptor{
            GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, pipeline.depthPrePass.subPasses[0].desc.attachments[0].handle};
        desc.depthMask = GL_FALSE;
        desc.depthFunc = GL_EQUAL;
        desc.clearBufferMask = GL_COLOR_BUFFER_BIT;
        desc.clearColor = {1, 1, 1, 1};

        auto const name = "Lighting";
        pipeline.lighting = CreateRenderPass(&desc, 1, programs.lighting, width, height, name, std::strlen(name));
    }

    {
        SubPassDescriptor desc;

        desc.attachmentCount = 2;
        desc.attachments[0] = SubPassAttachmentDescriptor{
            GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, CreateColorAttachment(width, height)};
        desc.attachments[1] = SubPassAttachmentDescriptor{
            GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, pipeline.depthPrePass.subPasses[0].desc.attachments[0].handle};
        desc.depthMask = GL_FALSE;
        desc.depthFunc = GL_EQUAL;
        desc.clearBufferMask = GL_COLOR_BUFFER_BIT;

        auto const name = "Velocity";
        pipeline.velocity = CreateRenderPass(&desc, 1, programs.velocity, width, height, name, std::strlen(name));
    }

    {
        SubPassDescriptor desc;
        desc.dependencyCount = 1;
        desc.dependencies[0] = SubPassDependencyDescriptor{
            GL_TEXTURE0,
            GL_TEXTURE_2D,
            pipeline.lighting.subPasses[0].desc.attachments[0].handle};
        desc.attachmentCount = 1;
        desc.attachments[0] = SubPassAttachmentDescriptor{
            GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, CreateColorAttachment(width, height)};
        desc.depthMask = GL_FALSE;
        desc.depthFunc = GL_ALWAYS;
        desc.clearBufferMask = GL_COLOR_BUFFER_BIT;

        auto const name = "Tone Mapping";
        pipeline.toneMapping = CreateRenderPass(&desc, 1, programs.toneMapping, width, height, name, std::strlen(name));
    }

    {
        auto const texture1 = CreateColorAttachment(width, height);
        auto const texture2 = CreateColorAttachment(width, height);

        SubPassDescriptor desc[2];

        desc[0].dependencyCount = 4;
        desc[0].dependencies[0] = SubPassDependencyDescriptor{
            GL_TEXTURE0, GL_TEXTURE_2D, pipeline.toneMapping.subPasses[0].desc.attachments[0].handle};
        desc[0].dependencies[1] = SubPassDependencyDescriptor{
            GL_TEXTURE1, GL_TEXTURE_2D, pipeline.lighting.subPasses[0].desc.attachments[1].handle};
        desc[0].dependencies[2] = SubPassDependencyDescriptor{GL_TEXTURE2, GL_TEXTURE_2D, texture2};
        desc[0].dependencies[3] = SubPassDependencyDescriptor{
            GL_TEXTURE3, GL_TEXTURE_2D, pipeline.velocity.subPasses[0].desc.attachments[0].handle};
        desc[0].attachmentCount = 1;
        desc[0].attachments[0] = SubPassAttachmentDescriptor{GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture1};
        desc[0].depthMask = GL_FALSE;
        desc[0].depthFunc = GL_ALWAYS;
        desc[0].clearBufferMask = GL_COLOR_BUFFER_BIT;

        desc[1].dependencyCount = desc[0].dependencyCount;
        desc[1].dependencies[0] = desc[0].dependencies[0];
        desc[1].dependencies[1] = desc[0].dependencies[1];
        desc[1].dependencies[2] = SubPassDependencyDescriptor{GL_TEXTURE2, GL_TEXTURE_2D, texture1};
        desc[1].dependencies[3] = desc[0].dependencies[3];
        desc[1].attachmentCount = 1;
        desc[1].attachments[0] = SubPassAttachmentDescriptor{GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture2};
        desc[1].depthMask = desc[0].depthMask;
        desc[1].depthFunc = desc[0].depthFunc;
        desc[1].clearBufferMask = desc[0].clearBufferMask;

        auto const name = "Temporal Pass";
        pipeline.taa = CreateRenderPass(desc, 2, programs.taa, width, height, name, std::strlen(name));
        pipeline.taa.subPasses[1].active = false;
    }

    {
        SubPassDescriptor desc;
        desc.dependencyCount = 6;
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
            pipeline.velocity.subPasses[0].desc.attachments[0].handle};
        desc.dependencies[3] = SubPassDependencyDescriptor{
            GL_TEXTURE3,
            GL_TEXTURE_2D,
            pipeline.toneMapping.subPasses[0].desc.attachments[0].handle};
        desc.dependencies[4] = SubPassDependencyDescriptor{
            GL_TEXTURE4,
            GL_TEXTURE_2D,
            pipeline.taa.subPasses[1].desc.attachments[0].handle};
        desc.dependencies[5] = SubPassDependencyDescriptor{
            GL_TEXTURE5,
            GL_TEXTURE_2D,
            pipeline.taa.subPasses[0].desc.attachments[0].handle};
        desc.attachmentCount = 1;
        desc.attachments[0] = SubPassAttachmentDescriptor{
            GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, CreateColorAttachment(width, height)};
        desc.depthMask = GL_FALSE;
        desc.depthFunc = GL_ALWAYS;
        desc.clearBufferMask = GL_COLOR_BUFFER_BIT;

        auto const name = "Debug";
        pipeline.debug = CreateRenderPass(&desc, 1, programs.debug, width, height, name, std::strlen(name));
    }

    return pipeline;
}

void DeleteRenderPipeline(Pipeline &pipeline)
{
    DeleteRenderPass(pipeline.depthPrePass);
    DeleteRenderPass(pipeline.shadowMapping);
    DeleteRenderPass(pipeline.lighting);
    DeleteRenderPass(pipeline.velocity);
    DeleteRenderPass(pipeline.toneMapping);
    DeleteRenderPass(pipeline.taa);
    DeleteRenderPass(pipeline.debug);
}

void UpdateGlobalUniforms(ShaderProgram const &program)
{
    for (auto const &uniform : program.ui32)
    {
        glUniform1uiv(uniform.location, uniform.count, uniform.data);
    }
    for (auto const &uniform : program.f1)
    {
        glUniform1fv(uniform.location, uniform.count, uniform.data);
    }
    for (auto const &uniform : program.f2)
    {
        glUniform2fv(uniform.location, uniform.count, uniform.data);
    }
    for (auto const &uniform : program.f3)
    {
        glUniform3fv(uniform.location, uniform.count, uniform.data);
    }
    for (auto const &uniform : program.f4)
    {
        glUniform4fv(uniform.location, uniform.count, uniform.data);
    }
    for (auto const &uniform : program.f16)
    {
        glUniformMatrix4fv(uniform.location, uniform.count, GL_TRUE, uniform.data);
    }
}

void UpdateLocalUniforms(ShaderProgram const &program, uint64_t index)
{
    for (auto const &uniform : program.ui32Array)
    {
        uint8_t const *data = reinterpret_cast<uint8_t const *>(uniform.data);
        data = data + uniform.offset + uniform.stride * index;
        glUniform1ui(uniform.location, *reinterpret_cast<uint32_t const *>(data));
    }
    for (auto const &uniform : program.f1Array)
    {
        uint8_t const *data = reinterpret_cast<uint8_t const *>(uniform.data);
        data = data + uniform.offset + uniform.stride * index;
        glUniform1f(uniform.location, *reinterpret_cast<float const *>(data));
    }
    for (auto const &uniform : program.f2Array)
    {
        uint8_t const *data = reinterpret_cast<uint8_t const *>(uniform.data);
        data = data + uniform.offset + uniform.stride * index;
        glUniform2fv(uniform.location, 1, reinterpret_cast<float const *>(data));
    }
    for (auto const &uniform : program.f3Array)
    {
        uint8_t const *data = reinterpret_cast<uint8_t const *>(uniform.data);
        data = data + uniform.offset + uniform.stride * index;
        glUniform3fv(uniform.location, 1, reinterpret_cast<float const *>(data));
    }
    for (auto const &uniform : program.f4Array)
    {
        uint8_t const *data = reinterpret_cast<uint8_t const *>(uniform.data);
        data = data + uniform.offset + uniform.stride * index;
        glUniform4fv(uniform.location, 1, reinterpret_cast<float const *>(data));
    }
    for (auto const &uniform : program.f16Array)
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
    glPushGroupMarkerEXT(pass.name.length, pass.name.data);

    for (uint8_t i = 0; i < pass.subPassCount; ++i)
    {
        auto &subPass = pass.subPasses[i];
        if (subPass.active)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, subPass.fbo);
            {
                glClearColor(subPass.desc.clearColor.r,
                             subPass.desc.clearColor.g,
                             subPass.desc.clearColor.b,
                             subPass.desc.clearColor.a);
                glClearDepth(subPass.desc.clearDepth);
                glDepthMask(subPass.desc.depthMask);
                glDepthFunc(subPass.desc.depthFunc);
                glViewport(0, 0, pass.width, pass.height);
                glScissor(0, 0, pass.width, pass.height);
                glClear(subPass.desc.clearBufferMask);

                glUseProgram(pass.program.handle);
                UpdateGlobalUniforms(pass.program);
                BindRenderPassDependencies(subPass.desc.dependencies, subPass.desc.dependencyCount);

                for (uint64_t j = 0; j < modelCount; ++j)
                {
                    UpdateLocalUniforms(pass.program, j);
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

    glPopGroupMarkerEXT();
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
