; First is default
LoadLanguageFile "${NSISDIR}\Contrib\Language files\Japanese.nlf"

; Language selection dialog
LangString InstallerLanguageTitle  ${LANG_JAPANESE} "インストーラの言語"
LangString SelectInstallerLanguage  ${LANG_JAPANESE} "インストーラの言語を選択してください"

; subtitle on license text caption
LangString LicenseSubTitleUpdate ${LANG_JAPANESE} " アップデート" 
LangString LicenseSubTitleSetup ${LANG_JAPANESE} " セットアップ" 

LangString MULTIUSER_TEXT_INSTALLMODE_TITLE ${LANG_JAPANESE} "インストールモード"
LangString MULTIUSER_TEXT_INSTALLMODE_SUBTITLE ${LANG_JAPANESE} "全てのユーザーにインストール（要管理者）しますか？それとも現在のユーザーだけにインストールしますか？"
LangString MULTIUSER_INNERTEXT_INSTALLMODE_TOP ${LANG_JAPANESE} "このインストーラを管理者の権限で実行する場合、次のいずれかへのインストールを選択できます。（例） c:\Program File または、現在のユーザーの AppData\ローカルフォルダ"
LangString MULTIUSER_INNERTEXT_INSTALLMODE_ALLUSERS ${LANG_JAPANESE} "全てのユーザーにインストール"
LangString MULTIUSER_INNERTEXT_INSTALLMODE_CURRENTUSER ${LANG_JAPANESE} "現在のユーザーだけにインストール"

; installation directory text
LangString DirectoryChooseTitle ${LANG_JAPANESE} "インストール・ディレクトリ" 
LangString DirectoryChooseUpdate ${LANG_JAPANESE} "アップデートするFirestormのディレクトリを選択してください。:" 
LangString DirectoryChooseSetup ${LANG_JAPANESE} "Firestormをインストールするディレクトリを選択してください。: " 

LangString MUI_TEXT_DIRECTORY_TITLE ${LANG_JAPANESE} "インストールディレクトリ"
LangString MUI_TEXT_DIRECTORY_SUBTITLE ${LANG_JAPANESE} "Firestorm をインストールするディレクトリを選択してください:"

LangString MUI_TEXT_INSTALLING_TITLE ${LANG_JAPANESE} "Firestorm をインストール中..."
LangString MUI_TEXT_INSTALLING_SUBTITLE ${LANG_JAPANESE} "$INSTDIR に Firestorm ビューワをインストール中"

LangString MUI_TEXT_FINISH_TITLE ${LANG_JAPANESE} "Firestorm をインストール中"
LangString MUI_TEXT_FINISH_SUBTITLE ${LANG_JAPANESE} "$INSTDIR に Firestorm ビューワをインストールしました。"

LangString MUI_TEXT_ABORT_TITLE ${LANG_JAPANESE} "インストールが中止されました"
LangString MUI_TEXT_ABORT_SUBTITLE ${LANG_JAPANESE} "$INSTDIR に Firestorm ビューワをインストールしません。"

; CheckStartupParams message box
LangString CheckStartupParamsMB ${LANG_JAPANESE} "プログラム吊'$INSTNAME'が見つかりません。サイレント・アップデートに失敗しました。" 

; installation success dialog
LangString InstSuccesssQuestion ${LANG_JAPANESE} "直ちにFirestormを開始しますか？ " 

; remove old NSIS version
LangString RemoveOldNSISVersion ${LANG_JAPANESE} "古いバージョン情報をチェック中です…" 

; check windows version
LangString CheckWindowsVersionDP ${LANG_JAPANESE} "ウィンドウズのバージョン情報をチェック中です..." 
LangString CheckWindowsVersionMB ${LANG_JAPANESE} "FirestormはWindows Vista SP2のみをサポートしています。" 
LangString CheckWindowsServPackMB ${LANG_JAPANESE} "It is recomended to run Firestorm on the latest service pack for your operating system.$\nThis will help with performance and stability of the program."
LangString UseLatestServPackDP ${LANG_JAPANESE} "Please use Windows Update to install the latest Service Pack."

; checkifadministrator function (install)
LangString CheckAdministratorInstDP ${LANG_JAPANESE} "インストールのための権限をチェック中です..." 
LangString CheckAdministratorInstMB ${LANG_JAPANESE} "Firestormをインストールするには管理者権限が必要です。"

; checkifadministrator function (uninstall)
LangString CheckAdministratorUnInstDP ${LANG_JAPANESE} "アンインストールのための権限をチェック中です..." 
LangString CheckAdministratorUnInstMB ${LANG_JAPANESE} "Firestormをアンインストールするには管理者権限が必要です。" 

; checkcpuflags
LangString MissingSSE2 ${LANG_JAPANESE} "This machine may not have a CPU with SSE2 support, which is required to run Firestorm ${VERSION_LONG}. Do you want to continue?"

; Extended cpu checks (AVX2)
LangString MissingAVX2 ${LANG_JAPANESE} "お使いのCPUはAVX2命令をサポートしていません。旧バージョンを次のリンクからダウンロードしてください: %DLURL%-legacy-cpus/"
LangString AVX2Available ${LANG_JAPANESE} "お使いのCPUはAVX2命令をサポートしています。より高いパフォーマンスを得るために、AVX2最適化版を次のリンクからダウンロードできます: %DLURL%/. 今すぐダウンロードしますか？"
LangString AVX2OverrideConfirmation ${LANG_JAPANESE} "If you believe that your PC can support AVX2 optimization, you may install anyway. Do you want to proceed?"
LangString AVX2OverrideNote ${LANG_JAPANESE} "By overriding the installer, you are about to install a version that might crash immediately after launching. If this happens, please immediately install the standard CPU version."

; closesecondlife function (install)
LangString CloseSecondLifeInstDP ${LANG_JAPANESE} "Firestormを終了中です..." 
LangString CloseSecondLifeInstMB ${LANG_JAPANESE} "Firestormの起動中にインストールは出来ません。直ちにFirestormを終了してインストールを開始する場合はOKボタンを押してください。CANCELを押すと中止します。"
LangString CloseSecondLifeInstRM ${LANG_JAPANESE} "Firestorm failed to remove some files from a previous install."

; closesecondlife function (uninstall)
LangString CloseSecondLifeUnInstDP ${LANG_JAPANESE} "Firestormを終了中です..." 
LangString CloseSecondLifeUnInstMB ${LANG_JAPANESE} "Firestormの起動中にアンインストールは出来ません。直ちにFirestormを終了してアンインストールを開始する場合はOKボタンを押してください。CANCELを押すと中止します。" 

; CheckNetworkConnection
LangString CheckNetworkConnectionDP ${LANG_JAPANESE} "ネットワークの接続を確認中..." 

; error during installation
LangString ErrorSecondLifeInstallRetry ${LANG_JAPANESE} "Firestorm installer encountered issues during installation. Some files may not have been copied correctly."
LangString ErrorSecondLifeInstallSupport ${LANG_JAPANESE} "Please reinstall viewer from https://www.firestormviewer.org/downloads/ and contact https://www.firestormviewer.org/support/ if issue persists after reinstall."

; ask to remove user's data files
LangString RemoveDataFilesMB ${LANG_JAPANESE} " Documents and Settings フォルダのキャッシュファイルをデリート中です？" 

; delete program files
LangString DeleteProgramFilesMB ${LANG_JAPANESE} "Firestormのディレクトリには、まだファイルが残されています。$\n$INSTDIR$\nにあなたが作成、または移動させたファイルがある可能性があります。全て削除しますか？ " 

; uninstall text
LangString UninstallTextMsg ${LANG_JAPANESE} "Firestorm${VERSION_LONG}をアンインストールします。"

; ask to remove registry keys that still might be needed by other viewers that are installed
LangString DeleteRegistryKeysMB ${LANG_JAPANESE} "Do you want to remove application registry keys?$\n$\nIt is recomended to keep registry keys if you have other versions of Firestorm installed."

; <FS:Ansariel> Ask to create protocol handler registry entries
LangString CreateUrlRegistryEntries ${LANG_JAPANESE} "Do you want to register Firestorm as default handler for virtual world protocols?$\n$\nIf you have other versions of Firestorm installed, this will overwrite the existing registry keys."

; <FS:Ansariel> Optional start menu entry
LangString CreateStartMenuEntry ${LANG_JAPANESE} "Create an entry in the start menu?"

; <FS:Ansariel> Application name suffix for OpenSim variant
LangString ForOpenSimSuffix ${LANG_JAPANESE} "for OpenSimulator"

LangString DeleteDocumentAndSettingsDP ${LANG_JAPANESE} "Deleting files in Documents and Settings folder."
LangString UnChatlogsNoticeMB ${LANG_JAPANESE} "This uninstall will NOT delete your Firestorm chat logs and other private files. If you want to do that yourself, delete the Firestorm folder within your user Application data folder."
LangString UnRemovePasswordsDP ${LANG_JAPANESE} "Removing Firestorm saved passwords."

LangString MUI_TEXT_LICENSE_TITLE ${LANG_JAPANESE} "Vivox Voice System License Agreement"
LangString MUI_TEXT_LICENSE_SUBTITLE ${LANG_JAPANESE} "Additional license agreement for the Vivox Voice System components."
LangString MUI_INNERTEXT_LICENSE_TOP ${LANG_JAPANESE} "Please read the following license agreement carefully before proceeding with the installation:"
LangString MUI_INNERTEXT_LICENSE_BOTTOM ${LANG_JAPANESE} "You have to agree to the license terms to continue with the installation."
