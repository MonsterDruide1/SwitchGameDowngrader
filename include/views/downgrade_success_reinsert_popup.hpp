#pragma once

#include <borealis/applet_frame.hpp>
#include <borealis/label.hpp>

/*
class DowngradeSuccessPopup : public brls::AppletFrame {
public:
    DowngradeSuccessPopup() : brls::AppletFrame(true, true) {
        brls::Label* message = new brls::Label(
            brls::LabelStyle::REGULAR,
            "The game has been successfully downgraded.\nMake sure to reinsert your gamecard before launching the game.",
            true
        );
        this->setContentView(message);
    }
};
*/
#include <borealis/dialog.hpp>
class DowngradeSuccessPopup {
public:
    static void show(bool is_downgrade) {
        const char* text = is_downgrade ? 
            "The game has been successfully downgraded.\nMake sure to reinsert your gamecard before launching the game." :
            "The game has been successfully restored to the current version.\nMake sure to reinsert your gamecard before launching the game.";
        brls::Dialog* dialog = new brls::Dialog(text);
        dialog->addButton("OK", [dialog, is_downgrade](brls::View* view) {
            LOG_MSG_DEBUG("Downgrade success popup OK button pressed.");
            dialog->close([is_downgrade]() {
                if (is_downgrade) {
                    // for some reason, the focusStack breaks when popping twice in a row and skips one view
                    // so we manually push the current focus before popping
                    brls::Application::getFocusStack().push_back(brls::Application::getCurrentFocus());

                    brls::Application::popView(brls::ViewAnimation::SLIDE_RIGHT);
                }
            });
        });
        dialog->open();
    }
};
