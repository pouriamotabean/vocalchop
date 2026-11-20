#ifndef AppVersion
  #define AppVersion "0.1.0"
#endif

[Setup]
AppName=ReChop
AppVersion={#AppVersion}
AppPublisher=Pouria Motabean
DefaultDirName={commoncf64}\VST3
DisableDirPage=yes
DisableProgramGroupPage=yes
Uninstallable=yes
OutputDir=..\dist
OutputBaseFilename=ReChop-Setup-{#AppVersion}
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
Compression=lzma2
SolidCompression=yes

[Files]
Source: "..\build\ReChop_artefacts\Release\VST3\ReChop.vst3\*"; DestDir: "{commoncf64}\VST3\ReChop.vst3"; Flags: recursesubdirs ignoreversion

[InstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\ReChop.vst3"
