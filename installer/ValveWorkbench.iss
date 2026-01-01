; ValveWorkbench Inno Setup installer
; Version: 1.0.0
; Install mode: per-machine (Program Files)
; Includes: Qt deployed payload from dist\ValveWorkbench\...
; Desktop shortcut: yes
;
; Prerequisites:
; - Build a payload folder first by running: installer\package.ps1
; - Install Inno Setup 6+ and compile this .iss
;
; Optional prerequisite:
; - Place VC_redist.x64.exe at: installer\VC_redist.x64.exe
;   If present, the installer will run it silently.

#define AppName "ValveWorkbench"
#define AppVersion "1.0.0"
#define AppPublisher "AudioSmith"
#define AppExeName "ValveWorkbench.exe"

#define RepoRoot "C:\\Users\\lizar\\Documents\\ValveWorkbench"
#define PayloadDir RepoRoot + "\\dist\\ValveWorkbench"

; Compile-time guard: the payload folder must exist before building the installer.
; Generate it by running: installer\package.ps1
#if !DirExists(PayloadDir)
  #error "Payload folder not found: " + PayloadDir + "\r\nRun installer\\package.ps1 first to generate dist\\ValveWorkbench\\..."
#endif

[Setup]
AppId={{9F8C2C2E-CE2A-47C8-9EE7-0CBE5E7C6A6E}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
DefaultDirName={userdocs}\\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
OutputBaseFilename={#AppName}_Setup_{#AppVersion}
OutputDir={#RepoRoot}\\dist\\installer
Compression=lzma
SolidCompression=yes
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional icons:"; Flags: unchecked

[Files]
; Main payload (Qt + app + models + analyser.json + circuits + arduino)
Source: "{#PayloadDir}\\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

; Optional VC++ Redistributable bootstrapper. If you place it in installer\, it will be installed.
#if FileExists(RepoRoot + "\\installer\\VC_redist.x64.exe")
Source: "{#RepoRoot}\\installer\\VC_redist.x64.exe"; DestDir: "{tmp}"; Flags: ignoreversion
#endif

[Icons]
Name: "{autoprograms}\\{#AppName}"; Filename: "{app}\\{#AppExeName}"
Name: "{autodesktop}\\{#AppName}"; Filename: "{app}\\{#AppExeName}"; Tasks: desktopicon

[Run]
; Run VC++ redist silently if present.
#if FileExists(RepoRoot + "\\installer\\VC_redist.x64.exe")
Filename: "{tmp}\\VC_redist.x64.exe"; Parameters: "/install /quiet /norestart"; StatusMsg: "Installing Microsoft Visual C++ Runtime..."; Flags: waituntilterminated; Check: IsAdminInstallMode
#endif

; Optionally launch app after install (unchecked by default)
Filename: "{app}\\{#AppExeName}"; Description: "Launch {#AppName}"; Flags: nowait postinstall skipifsilent unchecked
