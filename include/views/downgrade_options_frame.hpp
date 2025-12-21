/*
 * data_transfer_task_frame.hpp
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

#include <string>
#ifndef __DOWNGRADE_OPTIONS_FRAME_HPP__
#define __DOWNGRADE_OPTIONS_FRAME_HPP__

#include "downgrade_data.h"
#include "downgrade_task_frame.hpp"

namespace nxdt::views
{
    class DowngradeOptionsFrame: public brls::AppletFrame
    {
        private:
            u64 title_id;
            brls::List *list = nullptr;

            brls::Button* downgradeButton = nullptr;
            brls::Button* undoDowngradeButton = nullptr;

        protected:
            /* Set class as non-copyable and non-moveable. */
            NON_COPYABLE(DowngradeOptionsFrame);
            NON_MOVEABLE(DowngradeOptionsFrame);

        public:
            DowngradeOptionsFrame(const TitleApplicationMetadata *item_app_metadata) : brls::AppletFrame(true, true), title_id(item_app_metadata->title_id)
            {
                /* Generate icon using the default image. */
                brls::Image *icon = new brls::Image();
                icon->setImage(BOREALIS_ASSET("icon/" APP_TITLE ".jpg"));
                icon->setScaleType(brls::ImageScaleType::SCALE);

                /* Set UI properties. */
                this->setTitle("Apply Downgrade");
                this->setIcon(icon);

                this->list = new brls::List();
                this->list->setSpacing(this->list->getSpacing() / 2);
                this->list->setMarginBottom(20);

                if (getDowngradeDataForTitle(this->title_id) == nullptr) {
                    brls::Label* label = new brls::Label(brls::LabelStyle::REGULAR, "WARNING: No downgrade data available for this title.", true);
                    label->setColor(nvgRGB(255, 0, 0));
                    this->list->addView(label);
                    brls::Label* info_label = new brls::Label(brls::LabelStyle::DESCRIPTION, "You may still attempt to downgrade this title, but it may lead to crashes on launch. If that happens, come back here and undo the downgrade.\n\nIf you are a developer or modder for this game and want to help me add support for this game, contact me (@MonsterDruide1) on Discord!", true);
                    this->list->addView(info_label);
                } else {
                    brls::Label* label = new brls::Label(brls::LabelStyle::REGULAR, "Good news: This title can be downgraded!", true);
                    label->setColor(nvgRGB(0, 255, 0));
                    this->list->addView(label);
                    brls::Label* info_label = new brls::Label(brls::LabelStyle::DESCRIPTION, "You can proceed to apply the downgrade. If you encounter crashes on launch, report them to @MonsterDruide1 via Discord.", true);
                    this->list->addView(info_label);
                }

                downgradeButton = new brls::Button();
                downgradeButton->setLabel("Downgrade!");
                downgradeButton->getClickEvent()->subscribe([this](brls::View* view) {
                    brls::Application::pushView(new DowngradeTaskFrame(this->title_id), brls::ViewAnimation::SLIDE_LEFT, false);
                });
                downgradeButton->setHeight(60);
                this->list->addView(downgradeButton);
                
                undoDowngradeButton = new brls::Button();
                undoDowngradeButton->setLabel("Undo Downgrade");
                undoDowngradeButton->getClickEvent()->subscribe([this](brls::View* view) {
                    #define R_PATH_DOESNT_EXIST (0x202)
                    FsFileSystem* fs = utilsGetSdCardFileSystemObject();

                    std::string romfs_path = get_romfs_path(this->title_id);
                    std::string exefs_path = get_exefs_path(this->title_id);
                    u8 sdmc_prefix_length = strlen("sdmc:");

                    int r = fsFsDeleteFile(fs, romfs_path.c_str() + sdmc_prefix_length);
                    if(R_FAILED(r) && r != R_PATH_DOESNT_EXIST)
                        throw std::runtime_error("failed to delete romfs.bin");

                    r = fsFsDeleteFile(fs, exefs_path.c_str() + sdmc_prefix_length);
                    if(R_FAILED(r) && r != R_PATH_DOESNT_EXIST)
                        throw std::runtime_error("failed to delete exefs.nsp");

                    const GameDowngradeData* downgrade_data = getDowngradeDataForTitle(this->title_id);
                    if (downgrade_data != nullptr) {
                        std::string patch_path = get_patch_path(downgrade_data);
                        r = fsFsDeleteFile(fs, patch_path.c_str() + sdmc_prefix_length);
                        if(R_FAILED(r) && r != R_PATH_DOESNT_EXIST)
                            throw std::runtime_error("failed to delete patch");
                    }

                    DowngradeSuccessPopup::show(false);
                    this->updateDowngradeState();
                });
                undoDowngradeButton->setHeight(60);
                this->list->addView(undoDowngradeButton);

                this->updateDowngradeState();
                
                this->setContentView(this->list);
            }

            bool isTitleDowngraded() {
                std::string romfs_path = get_romfs_path(this->title_id);
                std::string exefs_path = get_exefs_path(this->title_id);

                if (!utilsCheckIfFileExists(romfs_path.c_str()))
                    return false;
                if (!utilsCheckIfFileExists(exefs_path.c_str()))
                    return false;

                // only check for patch if we have downgrade data for this title and it requires a patch
                const GameDowngradeData* downgrade_data = getDowngradeDataForTitle(this->title_id);
                if (downgrade_data != nullptr && downgrade_data->patch_data != nullptr) {
                    std::string patch_path = get_patch_path(downgrade_data);
                    if (!utilsCheckIfFileExists(patch_path.c_str()))
                        return false;
                }

                return true;
            }

            void updateDowngradeState() {
                bool is_downgraded = isTitleDowngraded();

                downgradeButton->setState(!is_downgraded ? brls::ButtonState::ENABLED : brls::ButtonState::DISABLED);
                undoDowngradeButton->setState(is_downgraded ? brls::ButtonState::ENABLED : brls::ButtonState::DISABLED);
            }

            void willAppear(bool resetState = false) override {
                updateDowngradeState();
                brls::AppletFrame::willAppear(resetState);
            }
    };
}

#endif  /* __DOWNGRADE_OPTIONS_FRAME_HPP__ */
