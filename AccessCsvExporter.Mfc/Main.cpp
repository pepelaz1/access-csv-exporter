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
            constexpr DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
            CRect bounds(100, 100, 840, 560); // 740 x 460 client area
            ::AdjustWindowRectEx(&bounds, style, FALSE, 0);
            bounds.OffsetRect(100 - bounds.left, 100 - bounds.top);
            Create(nullptr, L"Access → CSV", style, bounds);
        }

    protected:
        afx_msg int OnCreate(LPCREATESTRUCT details)
        {
            if (CFrameWnd::OnCreate(details) == -1) return -1;
            titleFont_.CreatePointFont(190, L"Segoe UI Semibold");
            subtitleFont_.CreatePointFont(100, L"Segoe UI");
            font_.CreatePointFont(100, L"Segoe UI");
            labelFont_.CreatePointFont(95, L"Segoe UI Semibold");
            headerBrush_.CreateSolidBrush(RGB(23, 43, 73));
            cardBrush_.CreateSolidBrush(RGB(255, 255, 255));
            pageBrush_.CreateSolidBrush(RGB(246, 248, 252));

            heading_.Create(L"Export Access data to CSV", WS_CHILD | WS_VISIBLE,
                CRect(28, 21, 690, 58), this);
            subtitle_.Create(L"Choose a database, select a table, and save the result.",
                WS_CHILD | WS_VISIBLE, CRect(29, 70, 690, 94), this);

            databaseLabel_.Create(L"ACCESS DATABASE  ·  .ACCDB / .MDB", WS_CHILD | WS_VISIBLE,
                CRect(42, 153, 680, 174), this);
            databaseEdit_.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL,
                CRect(42, 179, 564, 212), this, 2001);
            databaseBrowse_.Create(L"Browse...", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                CRect(577, 179, 698, 212), this, idDatabaseBrowse);

            tableLabel_.Create(L"TABLE", WS_CHILD | WS_VISIBLE,
                CRect(42, 230, 680, 251), this);
            tableCombo_.Create(WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
                CRect(42, 255, 564, 450), this, 2002);
            loadTables_.Create(L"Load tables", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                CRect(577, 255, 698, 288), this, idTableLoad);

            outputLabel_.Create(L"OUTPUT CSV  ·  UTF-8", WS_CHILD | WS_VISIBLE,
                CRect(42, 306, 680, 327), this);
            outputEdit_.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL,
                CRect(42, 331, 564, 364), this, 2003);
            outputBrowse_.Create(L"Browse...", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                CRect(577, 331, 698, 364), this, idOutputBrowse);

            exportButton_.Create(L"Export CSV", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                CRect(22, 400, 190, 440), this, idExport);
            exportButton_.EnableWindow(FALSE);
            status_.Create(L"Start by choosing an Access database file.",
                WS_CHILD | WS_VISIBLE, CRect(209, 403, 705, 439), this);

            heading_.SetFont(&titleFont_);
            subtitle_.SetFont(&subtitleFont_);
            CWnd* controls[] = { &databaseLabel_, &databaseEdit_, &databaseBrowse_,
                &tableLabel_, &tableCombo_, &loadTables_, &outputLabel_, &outputEdit_,
                &outputBrowse_, &exportButton_, &status_ };
            for (CWnd* control : controls) control->SetFont(&font_);
            for (CWnd* label : { static_cast<CWnd*>(&databaseLabel_),
                static_cast<CWnd*>(&tableLabel_), static_cast<CWnd*>(&outputLabel_) })
                label->SetFont(&labelFont_);
            return 0;
        }

        afx_msg BOOL OnEraseBkgnd(CDC* dc)
        {
            CRect area;
            GetClientRect(&area);
            dc->FillSolidRect(area, RGB(246, 248, 252));
            dc->FillSolidRect(0, 0, area.Width(), 116, RGB(23, 43, 73));
            dc->FillSolidRect(0, 113, area.Width(), 3, RGB(53, 145, 236));
            CRect card(22, 133, area.right - 22, 382);
            dc->FillSolidRect(card, RGB(255, 255, 255));
            dc->Draw3dRect(card, RGB(226, 232, 240), RGB(226, 232, 240));
            return TRUE;
        }

        afx_msg HBRUSH OnCtlColor(CDC* dc, CWnd* control, UINT kind)
        {
            if (kind == CTLCOLOR_STATIC)
            {
                dc->SetBkMode(TRANSPARENT);
                if (control == &heading_ || control == &subtitle_)
                {
                    dc->SetTextColor(control == &heading_ ? RGB(255, 255, 255) : RGB(188, 206, 229));
                    return headerBrush_;
                }
                if (control == &status_)
                {
                    dc->SetTextColor(RGB(69, 84, 105));
                    return pageBrush_;
                }
                dc->SetTextColor(RGB(55, 75, 104));
                return cardBrush_;
            }
            return CFrameWnd::OnCtlColor(dc, control, kind);
        }

        afx_msg void OnBrowseDatabase()
        {
            CFileDialog dialog(TRUE, L"accdb", nullptr,
                OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
                L"Access databases (*.accdb;*.mdb)|*.accdb;*.mdb|All files (*.*)|*.*||", this);
            if (dialog.DoModal() != IDOK) return;
            databaseEdit_.SetWindowText(dialog.GetPathName());
            tableCombo_.ResetContent();
            status_.SetWindowText(L"Database selected. Loading tables...");
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
                exportButton_.EnableWindow(!tables.empty());
                CString status;
                status.Format(L"Found %llu table(s). Choose an output file, then click Export CSV.",
                    static_cast<unsigned long long>(tables.size()));
                status_.SetWindowText(status);
                if (tables.empty()) AfxMessageBox(L"No user tables were found in this database.", MB_ICONINFORMATION);
            }
            catch (const std::exception& error)
            {
                exportButton_.EnableWindow(FALSE);
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
                message.Format(L"Done. Exported %llu row(s).",
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
        CFont font_, titleFont_, subtitleFont_, labelFont_;
        CBrush headerBrush_, cardBrush_, pageBrush_;
        CStatic heading_, subtitle_, databaseLabel_, tableLabel_, outputLabel_, status_;
        CEdit databaseEdit_, outputEdit_;
        CComboBox tableCombo_;
        CButton databaseBrowse_, loadTables_, outputBrowse_, exportButton_;
    };

    BEGIN_MESSAGE_MAP(MainWindow, CFrameWnd)
        ON_WM_CREATE()
        ON_WM_ERASEBKGND()
        ON_WM_CTLCOLOR()
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
