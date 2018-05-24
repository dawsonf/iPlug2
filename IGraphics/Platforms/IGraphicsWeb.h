#pragma once

#include <emscripten/val.h>
#include <emscripten/bind.h>

#include "IPlugPlatform.h"

#include "IGraphicsPathBase.h"
#include "IControl.h"

using namespace emscripten;

struct RetainVal
{
  RetainVal(val item) : mItem(item) {}
  val mItem;
};

class WebBitmap : public APIBitmap
{
public:
  WebBitmap(val imageCanvas, const char* name, int scale);
};

/** IGraphics draw/platform class HTML5 canvas
* @ingroup DrawClasses
* @ingroup PlatformClasses */
class IGraphicsWeb : public IGraphicsPathBase
{
  enum MouseState { kMouseStateUp, kMouseStateDownInside, kMouseStateDownOutside };

public:
  const char* GetDrawingAPIStr() override { return "WEB"; }

  IGraphicsWeb(IDelegate& dlg, int w, int h, int fps);
  ~IGraphicsWeb();

  void DrawBitmap(IBitmap& bitmap, const IRECT& bounds, int srcX, int srcY, const IBlend* pBlend) override;
  void DrawRotatedBitmap(IBitmap& bitmap, int destCentreX, int destCentreY, double angle, int yOffsetZeroDeg, const IBlend* pBlend) { IGraphicsPathBase::DrawRotatedBitmap(bitmap, destCentreX, destCentreY, DegToRad(angle), yOffsetZeroDeg, pBlend); }
  
  void PathClear() override { GetContext().call<void>("beginPath"); }
  void PathStart() override { GetContext().call<void>("beginPath"); } // TODO:
  void PathClose() override { GetContext().call<void>("closePath"); }

  void PathArc(float cx, float cy, float r, float aMin, float aMax) override { GetContext().call<void>("arc", cx, cy, r, DegToRad(aMin), DegToRad(aMax)); }

  void PathMoveTo(float x, float y) override { GetContext().call<void>("moveTo", x, y); }
  void PathLineTo(float x, float y) override { GetContext().call<void>("lineTo", x, y); }
  void PathCurveTo(float x1, float y1, float x2, float y2, float x3, float y3) override { GetContext().call<void>("bezierCurveTo", x1, y1, x2, y2, x3, y3); }

  void PathStroke(const IPattern& pattern, float thickness, const IStrokeOptions& options, const IBlend* pBlend) override;
  void PathFill(const IPattern& pattern, const IFillOptions& options, const IBlend* pBlend) override;

  void PathStateSave() override { GetContext().call<void>("save"); }
  void PathStateRestore() override {  GetContext().call<void>("restore"); }

  void PathTransformTranslate(float x, float y) override { GetContext().call<void>("translate", x, y); }
  void PathTransformScale(float scaleX, float scaleY) override { GetContext().call<void>("scale", scaleX, scaleY); }
  void PathTransformRotate(float angle) override { GetContext().call<void>("rotate", angle); }

  IColor GetPoint(int x, int y) override {} // TODO:
  void* GetData() override {} // TODO:

  bool DrawText(const IText& text, const char* str, IRECT& bounds, bool measure) override;
  bool MeasureText(const IText& text, const char* str, IRECT& bounds) override;

  void RenderDrawBitmap() override { /* Nothing to do here */ }

  void SetPlatformContext(void* pContext) override {} // TODO:

  void HideMouseCursor(bool hide, bool returnToStartPos) override
  {
    if(hide)
      val::global("document")["body"]["style"].set("cursor", std::string("none"));
    else
      val::global("document")["body"]["style"].set("cursor", std::string("auto"));
  }

  void ForceEndUserEdit() override {} // TODO:
  void Resize(int w, int h, float scale) override;
  void* OpenWindow(void* pParentWnd) override {} // TODO:
  void CloseWindow() override {} // TODO:
  void* GetWindow() override {} // TODO:
  bool WindowIsOpen() override { return GetWindow(); } // TODO: ??
  bool GetTextFromClipboard(WDL_String& str) override {} // TODO:
  void UpdateTooltips() override {} // TODO:
  int ShowMessageBox(const char* str, const char* caption, int type) override {} // TODO:
  
  IPopupMenu* CreatePopupMenu(IPopupMenu& menu, const IRECT& bounds, IControl* pCaller) override
  {
    ReleaseMouseCapture();
    
    assert(mPopupControl != nullptr);
    
    mPopupControl->CreatePopupMenu(menu, bounds, pCaller);
  }
  
  void CreateTextEntry(IControl& control, const IText& text, const IRECT& bounds, const char* str = "") override {} // TODO:
  void PromptForFile(WDL_String& filename, WDL_String& path, EFileAction action = kFileOpen, const char* extensions = 0) override {} // TODO:
  void PromptForDirectory(WDL_String& path) override {} // TODO:
  bool PromptForColor(IColor& color, const char* str = "") override {} // TODO:
  bool OpenURL(const char* url, const char* msgWindowTitle = 0, const char* confirmMsg = 0, const char* errMsgOnFailure = 0) override {} // TODO:
  const char* GetPlatformAPIStr() override { return "WEB"; }

  void OnMouseEvent(val event, bool outside);
  void OnKeyEvent(val event);
  
  static void OnMainLoopTimer();
protected:
  APIBitmap* LoadAPIBitmap(const WDL_String& resourcePath, int scale) override;
  APIBitmap* ScaleAPIBitmap(const APIBitmap* pBitmap, int scale) override;
  bool OSFindResource(const char* name, const char* type, WDL_String& result) override;

private:
  void ClipRegion(const IRECT& r) override;
  void ResetClipRegion() override;

  val GetCanvas()
  {
    return val::global("document").call<val>("getElementById", std::string("canvas"));
  }

  val GetContext()
  {
    val canvas = GetCanvas();
    return canvas.call<val>("getContext", std::string("2d"));
  }

  void SetWebSourcePattern(const IPattern& pattern, const IBlend* pBlend = nullptr);
  void SetWebBlendMode(const IBlend* pBlend);

  RetainVal* mPreloadedImages;
  RetainVal* mWindowListener = nullptr;
  MouseState mMouseState = kMouseStateUp;
  double mLastX = -1;
  double mLastY = -1;
  double mPositionL = -1;
  double mPositionT = -1;
};