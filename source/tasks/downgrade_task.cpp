/*
 * gamecard_image_dump_task.cpp
 *
 * Copyright (c) 2020-2024, DarkMatterCore <pabloacurielz@gmail.com>.
 *
 * This file is part of nxdumptool (https://github.com/DarkMatterCore/nxdumptool).
 *
 * nxdumptool is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * nxdumptool is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <tasks/downgrade_task.hpp>
#include <utils/scope_guard.hpp>
#include <utils/file_writer.hpp>
#include <core/gamecard.h>
#include <core/nacp.h>
#include <core/pfs.h>
#include "downgrade_data.h"

namespace i18n = brls::i18n;    /* For getStr(). */
using namespace i18n::literals; /* For _i18n. */

namespace nxdt::tasks
{
    DowngradeTaskError DowngradeTask::DoInBackground(const u64& title_id)
    {
        std::scoped_lock lock(this->task_mtx);

        DataTransferProgress progress{};

        std::string romfs_path = get_romfs_path(title_id);
        std::string exefs_path = get_exefs_path(title_id);

        LOG_MSG_DEBUG("Retrieving user application data for title %016lX.", title_id);
        TitleUserApplicationData user_app_data = {0};
        if(!titleGetUserApplicationData(title_id, &user_app_data) || !user_app_data.app_info) {
            LOG_MSG_DEBUG("Failed, showing error: Game seems to not be fully installed, missing your gamecard?");
            return "Game seems to not be fully installed, missing your gamecard?";
        }
        ON_SCOPE_EXIT { titleFreeUserApplicationData(&user_app_data); };
        LOG_MSG_DEBUG("Retrieved user application data for title %016lX.", title_id);

        u32 program_count = titleGetContentCountByType(user_app_data.app_info, NcmContentType_Program);
        if (!program_count)
            return "Base app has no program ncas!";
        LOG_MSG_DEBUG("Title %016lX has %u program ncas.", title_id, program_count);

        u8 program_id_offset = get_program_id_offset(title_id);
        if (program_id_offset >= program_count)
            return "Failed to get program id offset.";

        LOG_MSG_DEBUG("Starting downgrade for title %016lX.", title_id);

        NcaContext base_nca_ctx = {0};
        if (!ncaInitializeContext(
            &base_nca_ctx,
            user_app_data.app_info->storage_id,
            (user_app_data.app_info->storage_id == NcmStorageId_GameCard ? HashFileSystemPartitionType_Secure : HashFileSystemPartitionType_None),
            &(user_app_data.app_info->meta_key),
            titleGetContentInfoByTypeAndIdOffset(user_app_data.app_info, NcmContentType_Program, program_id_offset),
            NULL
        ))
            return "Failed to initialize base NCA context.";

        RomFileSystemContext romfs_ctx = {};
        if (!romfsInitializeContext(&romfs_ctx, &(base_nca_ctx.fs_ctx[1]), NULL))
            return "Failed to initialize RomFS context.";

        PartitionFileSystemContext exefs_ctx = {};
        if (!pfsInitializeContext(&exefs_ctx, &(base_nca_ctx.fs_ctx[0])))
            return "Failed to initialize ExeFS context.";

        utilsCreateDirectoryTree(romfs_path.c_str(), false);
        utilsCreateDirectoryTree(exefs_path.c_str(), false);

        if (!utilsCreateConcatenationFileWithSize(romfs_path.c_str(), romfs_ctx.size))
            return "Failed to create RomFS output file.";

        progress.total_size = romfs_ctx.size + exefs_ctx.size;
        this->PublishProgress(progress);

        utilsSetLongRunningProcessState(true);
        ON_SCOPE_EXIT { utilsSetLongRunningProcessState(false); };

        if (auto err = this->dumpSection(&(base_nca_ctx.fs_ctx[0]), exefs_ctx.offset, exefs_ctx.size, exefs_path.c_str(), progress); err)
            return err;
        if (auto err = this->dumpSection(&(base_nca_ctx.fs_ctx[1]), romfs_ctx.offset, romfs_ctx.size, romfs_path.c_str(), progress); err)
            return err;
        if (const GameDowngradeData* item_app_metadata = getDowngradeDataForTitle(title_id); item_app_metadata != nullptr) {
            std::string patch_path = get_patch_path(item_app_metadata);
            utilsCreateDirectoryTree(patch_path.c_str(), false);
            if (auto err = this->addPatch(patch_path.c_str(), item_app_metadata->patch_data, item_app_metadata->patch_size); err)
                return err;
        }
        progress.xfer_size = progress.total_size;
        progress.percentage = 100;
        this->PublishProgress(progress);

        return {};
    }

    DowngradeTaskError DowngradeTask::dumpSection(NcaFsSectionContext* section_ctx, u64 ctx_offset, u64 ctx_size, const std::string& output_path, DataTransferProgress& progress)
    {
        void* buf = usbAllocatePageAlignedBuffer(USB_TRANSFER_BUFFER_SIZE);
        if (!buf) return "Failed to allocate memory buffer.";
        ON_SCOPE_EXIT { free(buf); };
        
        nxdt::utils::FileWriter* file = nullptr;
        try {
            file = new nxdt::utils::FileWriter(output_path, ctx_size);
        } catch(const std::string& msg) {
            LOG_MSG_ERROR("%s", msg.c_str());
            return msg;
        }
        ON_SCOPE_EXIT { delete file; };

        for(size_t offset = 0, blksize = USB_TRANSFER_BUFFER_SIZE; offset < ctx_size; offset += blksize)
        {
            if (this->IsCancelled()) return {};

            if (blksize > (ctx_size - offset)) blksize = (ctx_size - offset);

            if (!ncaReadFsSection(section_ctx, buf, blksize, offset + ctx_offset))
                return "Failed to read section data.";

            if (!file->Write(buf, blksize))
                return "Failed to write section data.";

            progress.xfer_size += blksize;
            progress.percentage = static_cast<int>((progress.xfer_size * 100) / progress.total_size);
            this->PublishProgress(progress);
        }

        return {};
    }

    DowngradeTaskError DowngradeTask::addPatch(const char* patch_path, const u8* patch_data, u64 patch_size)
    {
        nxdt::utils::FileWriter* file = nullptr;
        try {
            file = new nxdt::utils::FileWriter(patch_path, patch_size);
        } catch(const std::string& msg) {
            LOG_MSG_ERROR("%s", msg.c_str());
            return msg;
        }
        ON_SCOPE_EXIT { delete file; };

        if (!file->Write(patch_data, patch_size))
            return "Failed to write patch data.";

        return {};
    }
}
