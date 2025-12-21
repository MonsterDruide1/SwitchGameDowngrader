/*
 * gamecard_image_dump_task.hpp
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

#pragma once

#ifndef __DOWNGRADE_TASK_HPP__
#define __DOWNGRADE_TASK_HPP__

#include <optional>
#include <mutex>

#include "data_transfer_task.hpp"
#include "../core/title.h"
#include "../core/nca.h"
#include "downgrade_data.h"

namespace nxdt::tasks
{
    typedef std::optional<std::string> DowngradeTaskError;

    /* Generates an image dump out of the inserted gamecard. */
    class DowngradeTask: public DataTransferTask<DowngradeTaskError, u64>
    {
        private:
            std::mutex task_mtx;

        protected:
            /* Set class as non-copyable and non-moveable. */
            NON_COPYABLE(DowngradeTask);
            NON_MOVEABLE(DowngradeTask);

            /* Runs in the background thread. */
            DowngradeTaskError DoInBackground(const u64 &title_id) override final;
            DowngradeTaskError dumpSection(NcaFsSectionContext* section_ctx, u64 ctx_offset, u64 ctx_size, const std::string& output_path, DataTransferProgress& progress);
            DowngradeTaskError addPatch(const char* patch_path, const u8* patch_data, u64 patch_size);

        public:
            DowngradeTask() = default;
    };
}

#endif  /* __DOWNGRADE_TASK_HPP__ */
