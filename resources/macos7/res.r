#include "MacTypes.r"

resource 'SIZE' (-1) {
    0, // dontSaveScreen
    1, // acceptSuspendResumeEvents
    0, // enableOpenableFinderInfo
    1, // backgroundAndForeground
    1, // canBackground
    1, // multiFinderAware
    0, // backgroundSwitch
    0, // dontTurnOffTransmitAndReceive
    1, // is32BitCompatible
    1, // isHighLevelEventAware
    0, // localAndRemoteHLEvents
    0, // isStationeryAware
    0, // dontUseTextEditServices
    0, // reserved
    0, // reserved
    0, // reserved
    16 * 1024 * 1024,   // 16MB recommended
    16 * 1024 * 1024    // 16MB minimum
};