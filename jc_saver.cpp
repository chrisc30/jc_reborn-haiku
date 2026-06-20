/*
 *  jc_saver.cpp  —  Haiku screensaver entry point
 *
 *  Implements the BScreenSaver interface.  The game logic runs in a dedicated
 *  thread so that storyPlay()'s internal blocking loop doesn't stall the
 *  screensaver framework.  Each time Haiku calls Draw(), the latest composited
 *  frame (grOutputSfc) is copied into a BBitmap and drawn to the BView.
 *
 *  Preview mode runs the full game at reduced tick rate — same as full screen.
 *
 *  Build target:
 *      ~/config/non-packaged/add-ons/Screen Savers/JohnnyCastaway
 */

#include <ScreenSaver.h>
#include <View.h>
#include <Bitmap.h>
#include <OS.h>
#include <Autolock.h>
#include <Locker.h>
#include <Message.h>
#include <String.h>
#include <StringView.h>
#include <TextControl.h>
#include <Button.h>
#include <LayoutBuilder.h>
#include <unistd.h>
#include "preview_image.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "preview_image.h"

extern "C" {
#include "mytypes.h"
#include "utils.h"
#include "resource.h"
#include "graphics.h"
#include "events.h"
#include "sound.h"
#include "story.h"
}

/* ------------------------------------------------------------------ */
/* Default resource path                                               */
/* ------------------------------------------------------------------ */
#define DEFAULT_RESOURCE_PATH  "/boot/home/JohnnyCastaway"

/* Message constants for the config panel */
#define MSG_PATH_CHANGED  'pthc'

/* ------------------------------------------------------------------ */
/* JohnnyScreenSaver                                                   */
/* ------------------------------------------------------------------ */

class JohnnyScreenSaver : public BScreenSaver {
public:
                    JohnnyScreenSaver(BMessage *archive, image_id id);
    virtual         ~JohnnyScreenSaver();

    virtual status_t StartSaver(BView *view, bool preview);
    virtual void     StopSaver();
    virtual void     Draw(BView *view, int32 frame);

    virtual void     StartConfig(BView *configView);
    virtual status_t SaveState(BMessage *into) const;
    virtual void     RestoreState(BMessage *from);

private:
    static int32     _GameThread(void *cookie);
    void             _StartGameThread();
    void             _StopGameThread();
    bool             _InitResources();

    thread_id        fGameThread;
    BLocker          fFrameLock;
    BBitmap         *fBitmap;
    bool             fRunning;
    bool             fPreview;
    bool             fResourcesLoaded;
    BString          fResourcePath;
};


/* ------------------------------------------------------------------ */
/* C export                                                            */
/* ------------------------------------------------------------------ */

extern "C" BScreenSaver *instantiate_screen_saver(BMessage *msg, image_id id)
{
    return new JohnnyScreenSaver(msg, id);
}


/* ------------------------------------------------------------------ */
/* Constructor / Destructor                                            */
/* ------------------------------------------------------------------ */

JohnnyScreenSaver::JohnnyScreenSaver(BMessage *archive, image_id id)
    : BScreenSaver(archive, id),
      fGameThread(-1),
      fBitmap(NULL),
      fRunning(false),
      fPreview(false),
      fResourcesLoaded(false)
{
    fResourcePath = DEFAULT_RESOURCE_PATH;
    RestoreState(archive);
}

JohnnyScreenSaver::~JohnnyScreenSaver()
{
    _StopGameThread();
    delete fBitmap;
}


/* ------------------------------------------------------------------ */
/* Resource initialisation (shared between preview and full screen)   */
/* ------------------------------------------------------------------ */

bool JohnnyScreenSaver::_InitResources()
{
    if (fResourcesLoaded) return true;

    BString mapPath(fResourcePath);
    mapPath << "/RESOURCE.MAP";

    chdir(fResourcePath.String());
    parseResourceFiles(const_cast<char*>(mapPath.String()));
    graphicsInit();
    soundInit();

    fResourcesLoaded = true;
    return true;
}


/* ------------------------------------------------------------------ */
/* BScreenSaver hooks                                                  */
/* ------------------------------------------------------------------ */

status_t JohnnyScreenSaver::StartSaver(BView *view, bool preview)
{
    fPreview = preview;

    if (preview) {
        BRect pframe(0, 0, 119, 89);
        fBitmap = new BBitmap(pframe, B_RGB32);
        if (fBitmap && fBitmap->IsValid() && fBitmap->Bits())
            memcpy(fBitmap->Bits(), preview_data, PREVIEW_WIDTH * PREVIEW_HEIGHT * 4);
        SetTickSize(1000000LL);
        return B_OK;
    }
    /* Allocate the BBitmap used by Draw() */
    BRect frame(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
    fBitmap = new BBitmap(frame, B_RGB32);
    if (!fBitmap || !fBitmap->IsValid())
        return B_NO_MEMORY;

    if (!_InitResources())
        return B_ERROR;

    evQuitRequested = 0;
    SetTickSize(20000LL);
    _StartGameThread();
    return B_OK;
}

void JohnnyScreenSaver::StopSaver()
{
    if (!fPreview) {
        _StopGameThread();
        soundEnd();
        graphicsEnd();
        fResourcesLoaded = false;
    }

    delete fBitmap;
    fBitmap = NULL;
}

void JohnnyScreenSaver::Draw(BView *view, int32 frame)
{
    if (fPreview) {

        if (fBitmap) {
            view->SetDrawingMode(B_OP_COPY);
            view->DrawBitmap(fBitmap, BPoint(0, 0));
            view->Flush();
        }
        return;
    }
    if (!fBitmap || !grOutputSfc)
        return;
    {
        BAutolock lock(fFrameLock);
        if (!lock.IsLocked()) return;
        memcpy(fBitmap->Bits(), grOutputSfc->pixels,
               SCREEN_WIDTH * SCREEN_HEIGHT * 4);
    }
    BRect src(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
    BRect dst = view->Bounds();
    view->DrawBitmap(fBitmap, src, dst);
    view->Flush();
}
void JohnnyScreenSaver::StartConfig(BView *configView)
{
    BRect b = configView->Bounds();
    FILE *cf=fopen("/boot/home/jc_cfg.log","w");if(cf){fprintf(cf,"bounds=%g x %g\n",b.right,b.bottom);fclose(cf);}

    /* Title */
    BStringView *title = new BStringView(
        BRect(10, 6, b.right - 10, 22),
        "title", "Johnny Castaway");
    title->SetFont(be_bold_font);
    configView->AddChild(title);

    /* Separator label */
    BStringView *sep = new BStringView(
        BRect(10, 26, b.right - 10, 40),
        "sep", "Resource path (RESOURCE.MAP folder):");
    configView->AddChild(sep);

    /* Path text field */
    BTextControl *pathField = new BTextControl(
        BRect(10, 42, b.right - 56, 60),
        "path_field", "", fResourcePath.String(),
        new BMessage(MSG_PATH_CHANGED));
    pathField->SetDivider(0);
    configView->AddChild(pathField);

    /* Apply button */
    BButton *applyBtn = new BButton(
        BRect(10, 64, 80, 84),
        "apply", "OK",
        new BMessage(MSG_PATH_CHANGED));
    configView->AddChild(applyBtn);

    /* Info line */
    BStringView *info = new BStringView(
        BRect(10, 100, b.right - 10, 114),
        "info",
        "© Sierra On-Line. Engine: Johnny Reborn.");
    configView->AddChild(info);
}

status_t JohnnyScreenSaver::SaveState(BMessage *into) const
{
    into->AddString("resource_path", fResourcePath.String());
    return B_OK;
}

void JohnnyScreenSaver::RestoreState(BMessage *from)
{
    if (!from) return;
    const char *path = NULL;
    if (from->FindString("resource_path", &path) == B_OK && path != NULL)
        fResourcePath = path;
}


/* ------------------------------------------------------------------ */
/* Game thread                                                         */
/* ------------------------------------------------------------------ */

int32 JohnnyScreenSaver::_GameThread(void *cookie)
{
    (void)cookie;
    storyPlay();
    return 0;
}

void JohnnyScreenSaver::_StartGameThread()
{
    fRunning    = true;
    fGameThread = spawn_thread(_GameThread, "johnny_story",
                               B_NORMAL_PRIORITY, this);
    if (fGameThread >= 0)
        resume_thread(fGameThread);
}

void JohnnyScreenSaver::_StopGameThread()
{
    evQuitRequested = 1;
    fRunning = false;

    if (fGameThread >= 0) {
        status_t result;
        wait_for_thread(fGameThread, &result);
        fGameThread = -1;
    }
}
// temp
