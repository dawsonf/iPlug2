#pragma once

#include <cstring>
#include <cstdint>

#include "ptrlist.h"
#include "mutex.h"

#include "IPlugPlatform.h"
#include "IPlugConstants.h"
#include "IPlugStructs.h"
#include "IPlugUtilities.h"
#include "IPlugParameter.h"

/**
 * @file
 * @copydoc IPlugBase
 * @defgroup APIClasses IPlug::APIClasses
*/

struct IPlugConfig;

/** The lowest level base class of an IPlug plug-in. No UI framework code included.  This interface does not handle audio processing, see @IPlugProcessor  */
class IPlugBase
{
public:
  IPlugBase(IPlugConfig config, EAPI plugAPI);
  virtual ~IPlugBase();

  virtual void OnParamChange(int paramIdx) {}
  
  // In case the audio processing thread needs to do anything when the GUI opens
  // (like for example, set some state dependent initial values for controls).
  virtual void OnGUIOpen() { TRACE; }
  virtual void OnGUIClose() { TRACE; }

  virtual void* OpenWindow(void* handle) { return nullptr; }
  virtual void CloseWindow() {} // plugin api asking to close window

  virtual bool MidiNoteName(int noteNumber, char* pNameStr) { *pNameStr = '\0'; return false; }

  virtual bool SerializeState(IByteChunk& chunk) { TRACE; return SerializeParams(chunk); }
  // Return the new chunk position (endPos). Implementations should call UnserializeParams() after custom data is unserialized
  virtual int UnserializeState(IByteChunk& chunk, int startPos) { TRACE; return UnserializeParams(chunk, startPos); }

  // Only used by AAX, override in plugins that do chunks
  virtual bool CompareState(const unsigned char* incomingState, int startPos);

  virtual void OnWindowResize() {}
  // implement this and return true to trigger your custom about box, when someone clicks about in the menu of a standalone
  virtual bool OnHostRequestingAboutBox() { return false; }

  // implement this to do something specific when IPlug is aware of the host
  // may get called multiple times
  virtual void OnHostIdentified() {}

  // ----------------------------------------
  // Your plugin class, or a control class, can call these functions.
  int NParams() const { return mParams.GetSize(); }
  IParam* GetParam(int paramIdx) { return mParams.Get(paramIdx); }

  const char* GetEffectName() const { return mEffectName.Get(); }\
  /** Get version code
   * @param decimal Sets the output format
   * @return Effect version in VVVVRRMM (if \p decimal is \c true) or 0xVVVVRRMM (if \p decimal is \c false) format
   */
  int GetEffectVersion(bool decimal) const;

  /** Get a printable version string
   * @param str WDL_String to write to
   * The output format is vX.M.m, where X - version, M - major, m - minor
   * @note If \c _DEBUG is defined, \c D is appended to the version string
   * @note If \c TRACER_BUILD is defined, \c T is appended to the version string
   */
  void GetEffectVersionStr(WDL_String& str) const;
  /** Get manufacturer name string */
  const char* GetMfrName() const { return mMfrName.Get(); }
  /** Get product name string */
  const char* GetProductName() const { return mProductName.Get(); }
  int GetUniqueID() const { return mUniqueID; }
  int GetMfrID() const { return mMfrID; }

  virtual void SetParameterInUIFromAPI(int paramIdx, double value, bool normalized) {}; // call from plugin API class to update GUI prior to calling OnParamChange();
  virtual void SetParameterFromUI(int idx, double normalizedValue); // called from GUI to update
  // If a parameter change comes from the GUI, midi, or external input,
  // the host needs to be informed in case the changes are being automated.
  virtual void BeginInformHostOfParamChange(int idx) = 0;
  virtual void InformHostOfParamChange(int idx, double normalizedValue) = 0;
  virtual void EndInformHostOfParamChange(int idx) = 0;
  virtual void InformHostOfProgramChange() = 0;

  virtual EHost GetHost() { return mHost; }
  virtual EAPI GetAPI() { return mAPI; }
  const char* GetAPIStr();
  const char* GetArchStr();
  
  /** @brief Used to get the build date of the plug-in and architecture/api details in one string
  * @note since the implementation is in IPlugBase.cpp, you may want to touch that file as part of your build script to force recompilation
  * @param str WDL_String will be set with the Plugin name, architecture, api, build date, build time*/
  void GetBuildInfoStr(WDL_String& str);
  int GetHostVersion(bool decimal); // Decimal = VVVVRRMM, otherwise 0xVVVVRRMM.
  void GetHostVersionStr(WDL_String& str);
  
  virtual bool GetHasUI() { return mHasUI; }
  virtual int GetUIWidth() { return 0; }
  virtual int GetUIHeight() { return 0; }

  // implement in API class to do something once editor is created/attached (called from IPlugBaseGraphics::AttachGraphics)
  virtual void OnGUICreated() {};

  // Tell the host that the graphics resized.
  virtual void ResizeGraphics(int w, int h, double scale) = 0;

  void EnsureDefaultPreset();
  
  int NParamGroups() { return mParamGroups.GetSize(); }
  const char* GetParamGroupName(int idx) { return mParamGroups.Get(idx); }
  int AddParamGroup(const char* name) { mParamGroups.Add(name); return NParamGroups(); }

  void InitChunkWithIPlugVer(IByteChunk& chunk);
  int GetIPlugVerFromChunk(IByteChunk& chunk, int& pos);

  void SetHost(const char* host, int version);   // Version = 0xVVVVRRMM.
  virtual void HostSpecificInit() {};


  
  // You can't use these three methods with chunks-based plugins, because there is no way to set the custom data
  void MakeDefaultPreset(const char* name = 0, int nPresets = 1);
  // MakePreset(name, param1, param2, ..., paramN)
  void MakePreset(const char* name, ...);
  // MakePresetFromNamedParams(name, nParamsNamed, paramEnum1, paramVal1, paramEnum2, paramVal2, ..., paramEnumN, paramVal2)
  // nParamsNamed may be less than the total number of params.
  void MakePresetFromNamedParams(const char* name, int nParamsNamed, ...);

  // Use these methods with chunks-based plugins
  void MakePresetFromChunk(const char* name, IByteChunk& chunk);
  void MakePresetFromBlob(const char* name, const char* blob, int sizeOfChunk);

  bool DoesStateChunks() { return mStateChunks; }

  // Will append if the chunk is already started
  bool SerializeParams(IByteChunk& chunk);
  int UnserializeParams(IByteChunk& chunk, int startPos); // Returns the new chunk position (endPos)

  virtual void RedrawParamControls() {};  // Called after restoring state.

  // ----------------------------------------
  // Internal IPlug stuff (but API classes need to get at it).

  void OnParamReset();  // Calls OnParamChange(each param) + OnReset().

  void PruneUninitializedPresets();

  // Unserialize / SerializePresets - Only used by VST2
  bool SerializePresets(IByteChunk& chunk);
  int UnserializePresets(IByteChunk& chunk, int startPos); // Returns the new chunk position (endPos).

  void ModifyCurrentPreset(const char* name = 0); // Sets the currently active preset to whatever current params are.
  int NPresets() { return mPresets.GetSize(); }
  int GetCurrentPresetIdx() { return mCurrentPresetIdx; }
  bool RestorePreset(int idx);
  bool RestorePreset(const char* name);
  const char* GetPresetName(int idx);

  virtual void* GetAAXViewInterface() { return nullptr; }

  // Dump the current state as source code for a call to MakePresetFromNamedParams / MakePresetFromBlob
  void DumpPresetSrcCode(const char* file, const char* paramEnumNames[]);
  void DumpPresetBlob(const char* file);
  void DumpBankBlob(const char* file);

  virtual void PresetsChangedByHost() {} // does nothing by default
  void DirtyParameters(); // hack to tell the host to dirty file state, when a preset is recalled

  //VST2 Presets
  bool SaveProgramAsFXP(const char* file);
  bool SaveBankAsFXB(const char* file);
  bool LoadProgramFromFXP(const char* file);
  bool LoadBankFromFXB(const char* file);
//  bool SaveBankAsFXPs(const char* path);

//   VST3 format
//   bool SaveProgramAsVSTPreset(const char* file);
//   bool LoadProgramFromVSTPreset(const char* file);
//   bool SaveBankAsVSTPresets(const char* path);
//
//   AU format
//   bool SaveProgramAsAUPreset(const char* name, const char* file);
//   bool LoadProgramFromAUPreset(const char* file);
//   bool SaveBankAsAUPresets(const char* path);
//
//   ProTools format
//   bool SaveProgramAsProToolsPreset(const char* presetName, const char* file, unsigned long pluginID);
//   bool LoadProgramFromProToolsPreset(const char* file);
//   bool SaveBankAsProToolsPresets(const char* bath, unsigned long pluginID);

  virtual void PrintDebugInfo();
  
  /** Effect name @todo WAT? */
  WDL_String mEffectName;
  /** Product name @todo WAT? */
  WDL_String mProductName;
  /** Manufacturer name */
  WDL_String mMfrName;

  //  Version stored as 0xVVVVRRMM: V = version, R = revision, M = minor revision.
  int mUniqueID;
  int mMfrID;
  int mVersion;
  int mHostVersion = 0;
  /**
   * @brief Plugin API */
  EAPI mAPI;
  EHost mHost = kHostUninit;

  WDL_PtrList<const char> mParamGroups;

  bool mStateChunks;

  /** \c True if the plug-in has a user interface. If false the host will provide a default interface */
  bool mHasUI = false;
  int mCurrentPresetIdx = 0;

  WDL_PtrList<IParam> mParams;
  WDL_PtrList<IPreset> mPresets;


  /** Lock when accessing mParams (including via GetParam) from the audio thread */
  WDL_Mutex mParams_mutex;
  WDL_String mParamDisplayStr = WDL_String("", MAX_PARAM_DISPLAY_LEN);
};
