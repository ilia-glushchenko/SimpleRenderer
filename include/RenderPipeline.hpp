/*
 * Copyright (C) 2019 by Ilya Glushchenko
 * This code is licensed under the MIT license (MIT)
 * (http://opensource.org/licenses/MIT)
 */
#pragma once

#include <RenderDefinitions.hpp>

//ToDo: I'm trying the no include thing, let's see
GLuint CreateDepthTexture(uint32_t width, uint32_t height);
GLuint CreateColorAttachment(int32_t width, int32_t height);
void DeleteRenderPass(RenderPass &pass);
RenderPass CreateRenderPass(SubPassDescriptor const *desc, uint8_t count,
                            ShaderProgram program,
                            int32_t width, int32_t height,
                            char const(name)[SHORT_STRING_MAX_LENGTH], uint8_t length);

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
        pipeline.depthPrePass = CreateRenderPass(
            &desc, 1, programs.depthPrePass, width, height, name, static_cast<uint8_t>(std::strlen(name)));
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
        pipeline.shadowMapping = CreateRenderPass(
            &desc, 1, programs.shadowMapping, 4096, 4096, name, static_cast<uint8_t>(std::strlen(name)));
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
        desc.depthFunc = GL_LEQUAL;
        desc.clearBufferMask = GL_COLOR_BUFFER_BIT;
        desc.clearColor = {1, 1, 1, 1};

        auto const name = "Lighting";
        pipeline.lighting = CreateRenderPass(
            &desc, 1, programs.lighting, width, height, name, static_cast<uint8_t>(std::strlen(name)));
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
            GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pipeline.lighting.subPasses[0].desc.attachments[0].handle};
        desc.attachments[1] = SubPassAttachmentDescriptor{
            GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, pipeline.depthPrePass.subPasses[0].desc.attachments[0].handle};
        desc.depthMask = GL_FALSE;
        desc.depthFunc = GL_LEQUAL;
        desc.clearBufferMask = GL_NONE_BIT;

        auto const name = "Transparency";
        pipeline.transparent = CreateRenderPass(
            &desc, 1, programs.transparent, width, height, name, static_cast<uint8_t>(std::strlen(name)));
    }

    {
        SubPassDescriptor desc;

        desc.attachmentCount = 2;
        desc.attachments[0] = SubPassAttachmentDescriptor{
            GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, CreateColorAttachment(width, height)};
        desc.attachments[1] = SubPassAttachmentDescriptor{
            GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, pipeline.depthPrePass.subPasses[0].desc.attachments[0].handle};
        desc.depthMask = GL_FALSE;
        desc.depthFunc = GL_LEQUAL;
        desc.clearBufferMask = GL_COLOR_BUFFER_BIT;

        auto const name = "Velocity";
        pipeline.velocity = CreateRenderPass(
            &desc, 1, programs.velocity, width, height, name, static_cast<uint8_t>(std::strlen(name)));
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
        pipeline.toneMapping = CreateRenderPass(
            &desc, 1, programs.toneMapping, width, height, name, static_cast<uint8_t>(std::strlen(name)));
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
        pipeline.taa = CreateRenderPass(
            desc, 2, programs.taa, width, height, name, static_cast<uint8_t>(std::strlen(name)));
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
        pipeline.debug = CreateRenderPass(
            &desc, 1, programs.debug, width, height, name, static_cast<uint8_t>(std::strlen(name)));
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
