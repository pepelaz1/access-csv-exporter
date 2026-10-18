#include <afxwin.h>
#include <afxdlgs.h>
#include <CommCtrl.h>

#include "OdbcCsvExporter.h"

#include <filesystem>
#include <string>

namespace
{
    constexpr UINT idDatabaseBrowse = 1001;
    constexpr UINT idTableLoad = 1002;
    constexpr UINT idOutputBrowse = 1003;
    constexpr UINT idExport = 1004;

    CString FromUtf8(const std::string& text)
    {
        const int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
            text.c_str(), -1, nullptr, 0);
        if (length <= 0) return L"An unexpected error occurred.";
        std::wstring wide(length, L'\0');
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
            text.c_str(), -1, wide.data(), length);
        return CString(wide.c_str());
    }

    class MainWindow final : public CFrameWnd
    {
    public:
        MainWindow()
        {
            Create(nullptr, L"Access to CSV — MFC sample",
                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                CRect(100, 100, 720, 435));
        }

    protected:
        afx_msg int OnCreate(LPCREATESTRUCT details)
        {
            if (CFrameWnd::OnCreate(details) == -1) return -1;
            font_.CreatePointFont(100, L"Segoe UI");

            heading_.Create(L"Microsoft Access → CSV", WS_CHILD | WS_VISIBLE,
                CRect(24, 17, 560, 48), this);
            databaseLabel_.Create(L"Access database (.accdb or .mdb)", WS_CHILD | WS_VISIBLE,
                CRect(24, 57, 560, 78), this);
            databaseEdit_.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                CRect(24, 81, 484, 108), this, 2001);
            databaseBrowse_.Create(L"Browse...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                CRect(495, 81, 585, 108), this, idDatabaseBrowse);

            tableLabel_.Create(L"Table", WS_CHILD | WS_VISIBLE,
                CRect(24, 126, 300, 147), this);
            tableCombo_.Create(WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
                CRect(24, 151, 484, 370), this, 2002);
            loadTables_.Create(L"Load tables", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                CRect(495, 151, 585, 178), this, idTableLoad);

            outputLabel_.Create(L"Output CSV (UTF-8)", WS_CHILD | WS_VISIBLE,
                CRect(24, 194, 560, 215), this);
            outputEdit_.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                CRect(24, 219, 484, 246), this, 2003);
            outputBrowse_.Create(L"Browse...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                CRect(495, 219, 585, 246), this, idOutputBrowse);

            exportButton_.Create(L"Export CSV", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                CRect(24, 272, 152, 306), this, idExport);
            status_.Create(L"Choose an Access file, load its tables, and select an output CSV.",
                WS_CHILD | WS_VISIBLE, CRect(170, 278, 585, 305), this);

            CWnd* controls[] = { &heading_, &databaseLabel_, &databaseEdit_, &databaseBrowse_,
                &tableLabel_, &tableCombo_, &loadTables_, &outputLabel_, &outputEdit_,
                &outputBrowse_, &exportButton_, &status_ };
            for (CWnd* control : controls) control->SetFont(&font_);
            return 0;
        }

        afx_msg void OnBrowseDatabase()
        {
            CFileDialog dialog(TRUE, L"accdb", nullptr,
                OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
                L"Access databases (*.accdb;*.mdb)|*.accdb;*.mdb|All files (*.*)|*.*||", this);
            if (dialog.DoModal() != IDOK) return;
            databaseEdit_.SetWindowText(dialog.GetPathName());
            tableCombo_.ResetContent();
            status_.SetWindowText(L"Database selected. Load its tables next.");
            try
            {
                const std::filesystem::path path(dialog.GetPathName().GetString());
                outputEdit_.SetWindowText((path.parent_path() /
                    (path.stem().wstring() + L".csv")).c_str());
            }
            catch (...) { /* The user can choose an output path manually. */ }
            OnLoadTables();
        }

        afx_msg void OnLoadTables()
        {
            CString database;
            databaseEdit_.GetWindowText(database);
            if (database.IsEmpty())
            {
                AfxMessageBox(L"Choose an Access database first.", MB_ICONINFORMATION);
                return;
            }
            try
            {
                const auto tables = access_csv::ListTables(database.GetString());
                tableCombo_.ResetContent();
                for (const auto& table : tables) tableCombo_.AddString(table.c_str());
                if (!tables.empty()) tableCombo_.SetCurSel(0);
                CString status;
                status.Format(L"Found %llu table(s).", static_cast<unsigned long long>(tables.size()));
                status_.SetWindowText(status);
                if (tables.empty()) AfxMessageBox(L"No user tables were found.", MB_ICONINFORMATION);
            }
            catch (const std::exception& error)
            {
                status_.SetWindowText(L"Could not load tables.");
                AfxMessageBox(FromUtf8(error.what()), MB_ICONERROR);
            }
        }

        afx_msg void OnBrowseOutput()
        {
            CFileDialog dialog(FALSE, L"csv", L"export.csv",
                OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY,
                L"CSV files (*.csv)|*.csv|All files (*.*)|*.*||", this);
            if (dialog.DoModal() == IDOK) outputEdit_.SetWindowText(dialog.GetPathName());
        }

        afx_msg void OnExport()
        {
            CString database, output, table;
            databaseEdit_.GetWindowText(database);
            outputEdit_.GetWindowText(output);
            const int selected = tableCombo_.GetCurSel();
            if (database.IsEmpty() || output.IsEmpty() || selected == CB_ERR)
            {
                AfxMessageBox(L"Choose a database, table, and output CSV file.", MB_ICONINFORMATION);
                return;
            }
            tableCombo_.GetLBText(selected, table);
            exportButton_.EnableWindow(FALSE);
            status_.SetWindowText(L"Exporting...");
            try
            {
                const std::size_t rows = access_csv::ExportTable(database.GetString(),
                    table.GetString(), output.GetString());
                CString message;
                message.Format(L"Exported %llu row(s) to CSV.",
                    static_cast<unsigned long long>(rows));
                status_.SetWindowText(message);
                AfxMessageBox(message, MB_ICONINFORMATION);
            }
            catch (const std::exception& error)
            {
                status_.SetWindowText(L"Export failed.");
                AfxMessageBox(FromUtf8(error.what()), MB_ICONERROR);
            }
            exportButton_.EnableWindow(TRUE);
        }

        DECLARE_MESSAGE_MAP()

    private:
        CFont font_;
        CStatic heading_, databaseLabel_, tableLabel_, outputLabel_, status_;
        CEdit databaseEdit_, outputEdit_;
        CComboBox tableCombo_;
        CButton databaseBrowse_, loadTables_, outputBrowse_, exportButton_;
    };

    BEGIN_MESSAGE_MAP(MainWindow, CFrameWnd)
        ON_WM_CREATE()
        ON_BN_CLICKED(idDatabaseBrowse, &MainWindow::OnBrowseDatabase)
        ON_BN_CLICKED(idTableLoad, &MainWindow::OnLoadTables)
        ON_BN_CLICKED(idOutputBrowse, &MainWindow::OnBrowseOutput)
        ON_BN_CLICKED(idExport, &MainWindow::OnExport)
    END_MESSAGE_MAP()

    class AccessCsvApp final : public CWinApp
    {
    public:
        BOOL InitInstance() override
        {
            INITCOMMONCONTROLSEX commonControls{ sizeof(INITCOMMONCONTROLSEX), ICC_WIN95_CLASSES };
            InitCommonControlsEx(&commonControls);
            CWinApp::InitInstance();
            auto* window = new MainWindow();
            if (!window->GetSafeHwnd())
            {
                delete window;
                return FALSE;
            }
            m_pMainWnd = window;
            window->ShowWindow(SW_SHOW);
            window->UpdateWindow();
            return TRUE;
        }
    };

    AccessCsvApp application;
}
