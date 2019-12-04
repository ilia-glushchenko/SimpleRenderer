/*
 * Copyright (C) 2019 by Ilya Glushchenko
 * This code is licensed under the MIT license (MIT)
 * (http://opensource.org/licenses/MIT)
 */
#pragma once

#include <RenderDefinitions.hpp>

//ToDo: I'm trying the no include thing, let's see
GLuint CreateDepthTexture(uint32_t width, uint32_t height);
GLuint CreateLinearColorAttachment(int32_t width, int32_t height);
void DeleteRenderPass(RenderPass &pass);
RenderPass CreateRenderPass(SubPassDescriptor const *desc, uint8_t count,
                            ShaderProgram program,
                            int32_t width, int32_t height,
                            char const(name)[SHORT_STRING_MAX_LENGTH], uint8_t length);

DeferredPipeline CreateDeferredRenderPipeline(DeferredPipelineShaderPrograms programs, int32_t width, int32_t height)
{
    DeferredPipeline pipeline = {};

    {
        SubPassDescriptor desc = CreateDefaultSubPassDescriptor();
        desc.attachmentCount = 1;
        desc.attachments[0] = SubPassAttachmentDescriptor{
            GL_DEPTH_ATTACHMENT,
            GL_TEXTURE_2D,
            CreateDepthTexture(width, height)};
        desc.enableWriteToDepth = true;
        desc.enableClearDepthBuffer = true;
        desc.depthTestFunction = GL_LESS;

        auto const name = "Depth Pre-pass";
        pipeline.depthPrePass = CreateRenderPass(
            &desc, 1, programs.depthPrePass, width, height, name, static_cast<uint8_t>(std::strlen(name)));
    }

    {
        SubPassDescriptor desc = CreateDefaultSubPassDescriptor();
        desc.attachmentCount = 4;
        //Position
        desc.attachments[0] = SubPassAttachmentDescriptor{
            GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D,
            CreateLinearColorAttachment(width, height)};
        //Normals
        desc.attachments[1] = SubPassAttachmentDescriptor{
            GL_COLOR_ATTACHMENT1,
            GL_TEXTURE_2D,
            CreatePointColorAttachment(width, height)};
        //Albedo
        desc.attachments[2] = SubPassAttachmentDescriptor{
            GL_COLOR_ATTACHMENT2,
            GL_TEXTURE_2D,
            CreateLinearColorAttachment(width, height)};
        //Depth
        desc.attachments[3] = SubPassAttachmentDescriptor{
            GL_DEPTH_ATTACHMENT,
            GL_TEXTURE_2D,
            pipeline.depthPrePass.subPasses[0].desc.attachments[0].handle};
        desc.depthTestFunction = GL_LEQUAL;
        desc.enableWriteToColor = true;
        desc.enableClearColorBuffer = true;

        auto const name = "GBuffer pass";
        pipeline.gBufferPass = CreateRenderPass(
            &desc, 1, programs.gBufferPass, width, height, name, static_cast<uint8_t>(std::strlen(name)));
    }

    {
        SubPassDescriptor desc = CreateDefaultSubPassDescriptor();
        desc.dependencyCount = 4;
        desc.dependencies[0] = SubPassDependencyDescriptor{
            GL_TEXTURE0,
            GL_TEXTURE_2D,
            pipeline.depthPrePass.subPasses[0].desc.attachments[0].handle};
        desc.dependencies[1] = SubPassDependencyDescriptor{
            GL_TEXTURE1,
            GL_TEXTURE_2D,
            pipeline.gBufferPass.subPasses[0].desc.attachments[0].handle};
        desc.dependencies[2] = SubPassDependencyDescriptor{
            GL_TEXTURE2,
            GL_TEXTURE_2D,
            pipeline.gBufferPass.subPasses[1].desc.attachments[1].handle};
        desc.dependencies[3] = SubPassDependencyDescriptor{
            GL_TEXTURE3,
            GL_TEXTURE_2D,
            pipeline.gBufferPass.subPasses[2].desc.attachments[2].handle};
        desc.attachmentCount = 1;
        desc.attachments[0] = SubPassAttachmentDescriptor{
            GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D,
            CreateLinearColorAttachment(width, height)};
        desc.enableWriteToColor = true;
        desc.enableClearColorBuffer = true;

        auto const name = "Lighting";
        pipeline.lighting = CreateRenderPass(
            &desc, 1, programs.lighting, width, height, name, static_cast<uint8_t>(std::strlen(name)));
    }

    {
        SubPassDescriptor desc = CreateDefaultSubPassDescriptor();
        desc.dependencyCount = 1;
        desc.dependencies[0] = SubPassDependencyDescriptor{
            GL_TEXTURE0,
            GL_TEXTURE_2D,
            pipeline.lighting.subPasses[0].desc.attachments[0].handle};
        desc.attachmentCount = 1;
        desc.attachments[0] = SubPassAttachmentDescriptor{
            GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D,
            CreateLinearColorAttachment(width, height)};
        desc.enableWriteToColor = true;
        desc.enableClearColorBuffer = true;

        auto const name = "Debug";
        pipeline.debug = CreateRenderPass(
            &desc, 1, programs.debug, width, height, name, static_cast<uint8_t>(std::strlen(name)));
    }

    return pipeline;
}

ForwardPipeline CreateForwardRenderPipeline(ForwardPipelineShaderPrograms programs, int32_t width, int32_t height)
{
    ForwardPipeline pipeline = {};

    {
        SubPassDescriptor desc = CreateDefaultSubPassDescriptor();
        desc.attachmentCount = 1;
        desc.attachments[0] = SubPassAttachmentDescriptor{
            GL_DEPTH_ATTACHMENT,
            GL_TEXTURE_2D,
            CreateDepthTexture(width, height)};
        desc.enableWriteToDepth = true;
        desc.enableClearDepthBuffer = true;
        desc.depthTestFunction = GL_LESS;

        auto const name = "Depth Pre-pass";
        pipeline.depthPrePass = CreateRenderPass(
            &desc, 1, programs.depthPrePass, width, height, name, static_cast<uint8_t>(std::strlen(name)));
    }

    {
        SubPassDescriptor desc = CreateDefaultSubPassDescriptor();
        desc.attachmentCount = 1;
        desc.attachments[0] = SubPassAttachmentDescriptor{
            GL_DEPTH_ATTACHMENT,
            GL_TEXTURE_2D,
            CreateDepthTexture(4096, 4096)};
        desc.enableWriteToDepth = true;
        desc.enableClearDepthBuffer = true;
        desc.depthTestFunction = GL_LESS;

        auto const name = "Shadow Mapping";
        pipeline.shadowMapping = CreateRenderPass(
            &desc, 1, programs.shadowMapping, 4096, 4096, name, static_cast<uint8_t>(std::strlen(name)));
    }

    {
        SubPassDescriptor desc = CreateDefaultSubPassDescriptor();
        desc.dependencyCount = 2;
        desc.dependencies[0] = SubPassDependencyDescriptor{
            GL_TEXTURE0,
            GL_TEXTURE_2D,
            pipeline.depthPrePass.subPasses[0].desc.attachments[0].handle};
        desc.dependencies[1] = SubPassDependencyDescriptor{
            GL_TEXTURE1,
            GL_TEXTURE_2D,
            pipeline.shadowMapping.subPasses[0].desc.attachments[0].handle};
        desc.attachmentCount = 2;
        desc.attachments[0] = SubPassAttachmentDescriptor{
            GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, CreateLinearColorAttachment(width, height)};
        desc.attachments[1] = SubPassAttachmentDescriptor{
            GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, pipeline.depthPrePass.subPasses[0].desc.attachments[0].handle};
        desc.depthTestFunction = GL_LEQUAL;
        desc.enableWriteToColor = true;
        desc.enableClearColorBuffer = true;

        auto const name = "Lighting";
        pipeline.lighting = CreateRenderPass(
            &desc, 1, programs.lighting, width, height, name, static_cast<uint8_t>(std::strlen(name)));
    }

    {
        SubPassDescriptor desc = CreateDefaultSubPassDescriptor();
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
        desc.enableWriteToColor = true;
        desc.enableClearColorBuffer = true;
        desc.depthTestFunction = GL_LEQUAL;

        auto const name = "Transparency";
        pipeline.transparent = CreateRenderPass(
            &desc, 1, programs.transparent, width, height, name, static_cast<uint8_t>(std::strlen(name)));
    }

    {
        SubPassDescriptor desc = CreateDefaultSubPassDescriptor();

        desc.attachmentCount = 2;
        desc.attachments[0] = SubPassAttachmentDescriptor{
            GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, CreatePointColorAttachment(width, height)};
        desc.attachments[1] = SubPassAttachmentDescriptor{
            GL_DEPTH_ATTACHMENT,
            GL_TEXTURE_2D,
            CreateDepthTexture(width, height)};
        desc.enableWriteToColor = true;
        desc.enableClearColorBuffer = true;
        desc.depthTestFunction = GL_LEQUAL;

        auto const name = "Velocity";
        pipeline.velocity = CreateRenderPass(
            &desc, 1, programs.velocity, width, height, name, static_cast<uint8_t>(std::strlen(name)));
    }

    {
        auto const texture1 = CreateLinearColorAttachment(width, height);
        auto const texture2 = CreateLinearColorAttachment(width, height);
        auto const debug_texture = CreateLinearColorAttachment(width, height);

        SubPassDescriptor desc[2] = {
            CreateDefaultSubPassDescriptor(),
            CreateDefaultSubPassDescriptor()};

        desc[0].dependencyCount = 4;
        desc[0].dependencies[0] = SubPassDependencyDescriptor{
            GL_TEXTURE0, GL_TEXTURE_2D, pipeline.lighting.subPasses[0].desc.attachments[0].handle};
        desc[0].dependencies[1] = SubPassDependencyDescriptor{
            GL_TEXTURE1, GL_TEXTURE_2D, pipeline.lighting.subPasses[0].desc.attachments[1].handle};
        desc[0].dependencies[2] = SubPassDependencyDescriptor{GL_TEXTURE2, GL_TEXTURE_2D, texture2};
        desc[0].dependencies[3] = SubPassDependencyDescriptor{
            GL_TEXTURE3, GL_TEXTURE_2D, pipeline.velocity.subPasses[0].desc.attachments[0].handle};
        desc[0].attachmentCount = 2;
        desc[0].attachments[0] = SubPassAttachmentDescriptor{GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture1};
        desc[0].attachments[1] = SubPassAttachmentDescriptor{GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, debug_texture};
        desc[0].enableWriteToColor = true;
        desc[0].enableClearColorBuffer = true;

        desc[1].dependencyCount = desc[0].dependencyCount;
        desc[1].dependencies[0] = desc[0].dependencies[0];
        desc[1].dependencies[1] = desc[0].dependencies[1];
        desc[1].dependencies[2] = SubPassDependencyDescriptor{GL_TEXTURE2, GL_TEXTURE_2D, texture1};
        desc[1].dependencies[3] = desc[0].dependencies[3];
        desc[1].attachmentCount = desc[0].attachmentCount;
        desc[1].attachments[0] = SubPassAttachmentDescriptor{GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture2};
        desc[1].attachments[1] = desc[0].attachments[1];
        desc[1].enableWriteToColor = desc[0].enableWriteToColor;
        desc[1].enableClearColorBuffer = desc[0].enableClearColorBuffer;

        auto const name = "Temporal Pass";
        pipeline.taa = CreateRenderPass(
            desc, 2, programs.taa, width, height, name, static_cast<uint8_t>(std::strlen(name)));
        pipeline.taa.subPasses[1].active = false;
    }

    {
        SubPassDescriptor desc = CreateDefaultSubPassDescriptor();
        desc.dependencyCount = 1;
        desc.dependencies[0] = SubPassDependencyDescriptor{
            GL_TEXTURE0,
            GL_TEXTURE_2D,
            pipeline.taa.subPasses[0].desc.attachments[0].handle};
        desc.attachmentCount = 1;
        desc.attachments[0] = SubPassAttachmentDescriptor{
            GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, CreateLinearColorAttachment(width, height)};
        desc.enableWriteToColor = true;
        desc.enableClearColorBuffer = true;

        auto const name = "Tone Mapping";
        pipeline.toneMapping = CreateRenderPass(
            &desc, 1, programs.toneMapping, width, height, name, static_cast<uint8_t>(std::strlen(name)));
    }

    {
        SubPassDescriptor desc = CreateDefaultSubPassDescriptor();
        desc.dependencyCount = 7;
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
        desc.dependencies[6] = SubPassDependencyDescriptor{
            GL_TEXTURE6,
            GL_TEXTURE_2D,
            pipeline.taa.subPasses[0].desc.attachments[1].handle};
        desc.attachmentCount = 1;
        desc.attachments[0] = SubPassAttachmentDescriptor{
            GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, CreateLinearColorAttachment(width, height)};
        desc.enableWriteToColor = true;
        desc.enableClearColorBuffer = true;

        auto const name = "Debug";
        pipeline.debug = CreateRenderPass(
            &desc, 1, programs.debug, width, height, name, static_cast<uint8_t>(std::strlen(name)));
    }

    return pipeline;
}

void DeleteRenderPipeline(RenderPass *passes, uint8_t count)
{
    for (uint8_t i = 0; i < count; ++i)
    {
        DeleteRenderPass(passes[i]);
    }
}
