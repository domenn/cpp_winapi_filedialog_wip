

#include <iostream>

void show_my()
{

}//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//



#include "windows_headers.h"

#define STRICT_TYPED_ITEMIDS
#include <shlobj.h>
#include <objbase.h>      // For COM headers
#include <shobjidl.h>     // for IFileDialogEvents and IFileDialogControlEvents
#include <shlwapi.h>
#include <knownfolders.h> // for KnownFolder APIs/datatypes/function headers
#include <propvarutil.h>  // for PROPVAR-related functions
#include <propkey.h>      // for the Property key APIs/datatypes
#include <propidl.h>      // for the Property System APIs
#include <strsafe.h>      // for StringCchPrintfW
#include <shtypes.h>      // for COMDLG_FILTERSPEC
#include <new>

#pragma comment(linker, "\"/manifestdependency:type='Win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma comment(lib,"comctl32.lib") 
#pragma comment(lib,"propsys.lib")
#pragma comment(lib,"shlwapi.lib") 



// Indices of file types
#define INDEX_WORDDOC 1
#define INDEX_WEBPAGE 2
#define INDEX_TEXTDOC 3

// Controls
#define CONTROL_GROUP           2000
#define CONTROL_RADIOBUTTONLIST 2
#define CONTROL_RADIOBUTTON1    1
#define CONTROL_RADIOBUTTON2    2       // It is OK for this to have the same ID as CONTROL_RADIOBUTTONLIST,
										// because it is a child control under CONTROL_RADIOBUTTONLIST

// IDs for the Task Dialog Buttons
#define IDC_BASICFILEOPEN                       100
#define IDC_ADDITEMSTOCUSTOMPLACES              101
#define IDC_ADDCUSTOMCONTROLS                   102
#define IDC_SETDEFAULTVALUESFORPROPERTIES       103
#define IDC_WRITEPROPERTIESUSINGHANDLERS        104
#define IDC_WRITEPROPERTIESWITHOUTUSINGHANDLERS 105

enum class ReleaseType { RELEASE, COTASKMEMFREE };

template <typename Ptype, ReleaseType TparRelType = ReleaseType::RELEASE>
struct Releasable
{
	Ptype* fd{};

	Releasable() = default;
	Releasable(const Releasable& other) = delete;

	Releasable& operator=(const Releasable& other) = delete;

	Releasable(Releasable&& other) noexcept : fd(other.fd)
	{
		other.fd = nullptr;
	}

	Releasable& operator=(Releasable&& other) noexcept = delete;

	~Releasable()
	{
		if (fd)
		{
			if constexpr (TparRelType == ReleaseType::RELEASE) {
				fd->Release();
			}
			else if constexpr (TparRelType == ReleaseType::COTASKMEMFREE) {
				CoTaskMemFree(fd);
			}
		}
	}
};

/* File Dialog Event Handler *****************************************************************************************************/
class CDialogEventHandler : public IFileDialogEvents,
	public IFileDialogControlEvents
{
public:
	// IUnknown methods
	IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv)
	{
		static const QITAB qit[] = {
			QITABENT(CDialogEventHandler, IFileDialogEvents),
			QITABENT(CDialogEventHandler, IFileDialogControlEvents),
			{ 0 },
		};
		return QISearch(this, qit, riid, ppv);
	}

	IFACEMETHODIMP_(ULONG) AddRef()
	{
		return InterlockedIncrement(&_cRef);
	}

	IFACEMETHODIMP_(ULONG) Release()
	{
		long cRef = InterlockedDecrement(&_cRef);
		if (!cRef)
			delete this;
		return cRef;
	}

	// IFileDialogEvents methods
	IFACEMETHODIMP OnFileOk(IFileDialog*);
	IFACEMETHODIMP OnFolderChange(IFileDialog*) ;
	IFACEMETHODIMP OnFolderChanging(IFileDialog*, IShellItem*) ;
	IFACEMETHODIMP OnHelp(IFileDialog*) { return S_OK; };
	IFACEMETHODIMP OnSelectionChange(IFileDialog*);
	IFACEMETHODIMP OnShareViolation(IFileDialog*, IShellItem*, FDE_SHAREVIOLATION_RESPONSE*) { return S_OK; };
	IFACEMETHODIMP OnTypeChange(IFileDialog* pfd);
	IFACEMETHODIMP OnOverwrite(IFileDialog*, IShellItem*, FDE_OVERWRITE_RESPONSE*);

	// IFileDialogControlEvents methods
	IFACEMETHODIMP OnItemSelected(IFileDialogCustomize* pfdc, DWORD dwIDCtl, DWORD dwIDItem);
	IFACEMETHODIMP OnButtonClicked(IFileDialogCustomize*, DWORD);
	IFACEMETHODIMP OnCheckButtonToggled(IFileDialogCustomize*, DWORD, BOOL) { return S_OK; };
	IFACEMETHODIMP OnControlActivating(IFileDialogCustomize*, DWORD) { return S_OK; };

	CDialogEventHandler() : _cRef(1) { };
private:
	~CDialogEventHandler() { };
	long _cRef;
};

// IFileDialogControlEvents
// This method gets called when an dialog control item selection happens (radio-button selection. etc).
// For sample sake, let's react to this event by changing the dialog title.
HRESULT CDialogEventHandler::OnItemSelected(IFileDialogCustomize* pfdc, DWORD dwIDCtl, DWORD dwIDItem)
{
	IFileDialog* pfd = NULL;
	HRESULT hr = pfdc->QueryInterface(&pfd);
	if (SUCCEEDED(hr))
	{
		if (dwIDCtl == CONTROL_RADIOBUTTONLIST)
		{
			switch (dwIDItem)
			{
			case CONTROL_RADIOBUTTON1:
				hr = pfd->SetTitle(L"Longhorn Dialog");
				break;

			case CONTROL_RADIOBUTTON2:
				hr = pfd->SetTitle(L"Vista Dialog");
				break;
			}
		}
		pfd->Release();
	}
	return hr;
}

HRESULT CDialogEventHandler::OnButtonClicked(IFileDialogCustomize* p1, DWORD p2)
{
	std::wcout << __FUNCTIONW__ << L"(" << __LINE__ << L")" << L" P2 " << p2 << L"\n";
	return S_OK;
}

HRESULT CDialogEventHandler::OnFileOk(IFileDialog* some_dialog)
{
	//Releasable<IShellItem> psiResult;
	//auto hr = some_dialog->GetResult(&psiResult.fd);
	//if (FAILED(hr))
	//{
	//	std::cout << "ME FAILED AT " << __FUNCTION__ << ";" << __LINE__;
	//	return hr;
	//}
	//Releasable<wchar_t, ReleaseType::COTASKMEMFREE> fn_output;
	//hr = psiResult.fd->GetDisplayName(SIGDN_FILESYSPATH, &fn_output.fd);
	//if (FAILED(hr))
	//{
	//	std::cout << "ME FAILED AT " << __FUNCTION__ << ";" << __LINE__;
	//	return hr;
	//}

	Releasable<wchar_t, ReleaseType::COTASKMEMFREE> fn_output;
	some_dialog->GetFileName(&fn_output.fd);
	
	std::wcout << __FUNCTIONW__ << L"(" << __LINE__ << L")" << L" Pre Transform " << fn_output.fd << L"\n";
	const auto ptr_last_char  = fn_output.fd + wcslen(fn_output.fd) - 1;
	const std::reverse_iterator<wchar_t*> rightmost(ptr_last_char);
	const std::reverse_iterator<wchar_t*> leftmost(fn_output.fd);
	const auto dot_where = std::find(rightmost, leftmost, L'.');
	if (dot_where != leftmost)
	{
		// Extension found
		// *(dot_where + 1) = 0;
		*(dot_where - 1) = L'Q';
		*(dot_where - 2) = L'd';
	}

	
	
	std::wcout << __FUNCTIONW__ << L"(" << __LINE__ << L")" << L" PostTransform " << fn_output.fd << L"\n";
	wchar_t* dumb = new wchar_t[40];
	StringCchCopyW(dumb, 40, fn_output.fd);
	auto hr = some_dialog->SetFileName(dumb);
	// some_dialog->
	return hr;
	return S_OK;
}

HRESULT CDialogEventHandler::OnFolderChange(IFileDialog*)
{
	std::wcout << __FUNCTIONW__ << L"(" << __LINE__ << L")" << L"\n";
	return S_OK;
}

HRESULT CDialogEventHandler::OnFolderChanging(IFileDialog*, IShellItem*)
{
	std::wcout << __FUNCTIONW__ << L"(" << __LINE__ << L")" << L"\n";
	return S_OK;
}

HRESULT CDialogEventHandler::OnSelectionChange(IFileDialog*)
{
	std::wcout << __FUNCTIONW__ << L"(" << __LINE__ << L")" << L"\n";
	return S_OK;
}

// IFileDialogEvents methods
// This method gets called when the file-type is changed (combo-box selection changes).
// For sample sake, let's react to this event by changing the properties show.
HRESULT CDialogEventHandler::OnTypeChange(IFileDialog* pfd)
{
	IFileSaveDialog* pfsd;
	HRESULT hr = pfd->QueryInterface(&pfsd);
	if (SUCCEEDED(hr))
	{
		UINT uIndex;
		hr = pfsd->GetFileTypeIndex(&uIndex);   // index of current file-type
		if (SUCCEEDED(hr))
		{
			std::wcout << __FUNCTIONW__ << L"(" << __LINE__ << L")" << L" GetFileTypeIndex returns " << uIndex << L"\n";
		}
		pfsd->Release();
	}
	if (FAILED(hr))
	{
		std::wcout << __FUNCTIONW__ << L"(" << __LINE__ << L")" << L" CALLED BUT HR FAILED!\n";
	}
	return hr;
}

HRESULT CDialogEventHandler::OnOverwrite(IFileDialog* dlg, IShellItem* shl_itm, FDE_OVERWRITE_RESPONSE* response_elem)
{
	std::wcout << __FUNCTIONW__ << L"(" << __LINE__ << L")" << L" OverwriteRequest; returning \"ALLOW\" \n";
	*response_elem = FDE_OVERWRITE_RESPONSE::FDEOR_ACCEPT;
	return S_OK;
}


HRESULT CDialogEventHandler_CreateInstance(REFIID riid, void** ppv)
{
	*ppv = NULL;
	CDialogEventHandler* pDialogEventHandler = new (std::nothrow) CDialogEventHandler();
	HRESULT hr = pDialogEventHandler ? S_OK : E_OUTOFMEMORY;
	if (SUCCEEDED(hr))
	{
		hr = pDialogEventHandler->QueryInterface(riid, ppv);
		pDialogEventHandler->Release();
	}
	return hr;
}




template <typename some_function>
struct CustomRelease
{
	some_function releaser;
	CustomRelease(some_function&& rf) : releaser(std::move(rf)) {}
	~CustomRelease()
	{
		releaser();
	}
};

const COMDLG_FILTERSPEC filters_pdf_or_ps[] =
{
	{L"PDF Documents(*.pdf)",       L"*.pdf"},
	{L"PostScript Documents(*.ps)",    L"*.ps"},
};

HRESULT DoStuffWithProvidedSaveFile(IFileDialog* const sfd)
{
	Releasable<IShellItem> psiResult;
	auto hr = sfd->GetResult(&psiResult.fd);
	if (FAILED(hr))
	{
		std::cout << "ME FAILED AT " << __FUNCTION__ << ";" << __LINE__;
		return hr;
	}
	Releasable<wchar_t, ReleaseType::COTASKMEMFREE> fn_output;
	hr = psiResult.fd->GetDisplayName(SIGDN_FILESYSPATH, &fn_output.fd);
	if (FAILED(hr))
	{
		std::cout << "ME FAILED AT " << __FUNCTION__ << ";" << __LINE__;
		return hr;
	}
	std::wcout << __FUNCTIONW__ << L"(" << __LINE__ << L")" << " Filename is " << fn_output.fd << L'\n';
}

std::tuple< HRESULT, Releasable<IFileDialogEvents>, DWORD>  hook_event_handler(IFileDialog* sfd)
{
	Releasable<IFileDialogEvents> pfde;
	auto hr = CDialogEventHandler_CreateInstance(IID_PPV_ARGS(&pfde.fd));
	if (FAILED(hr))
	{
		std::cout << "ME FAILED AT " << __FUNCTION__ << ";" << __LINE__;
		return std::make_tuple(hr, std::move(pfde), 0);
	}
	DWORD dwCookie;
	hr = sfd->Advise(pfde.fd, &dwCookie);
	return std::make_tuple(hr, std::move(pfde), dwCookie);
}

HRESULT BasicFileSave()
{
	Releasable<IFileDialog> sfd;
	HRESULT hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&sfd.fd));
	if (FAILED(hr))
	{
		std::cout << "ME FAILED AT " << __FUNCTION__ << ";" << __LINE__;
		return hr;
	}
	hr = sfd.fd->SetFileTypes(ARRAYSIZE(filters_pdf_or_ps), filters_pdf_or_ps);
	if (FAILED(hr))
	{
		std::cout << "ME FAILED AT " << __FUNCTION__ << ";" << __LINE__;
		return hr;
	}
	DWORD dwFlags;
	hr = sfd.fd->GetOptions(&dwFlags);
	if (FAILED(hr))
	{
		std::cout << "ME FAILED AT " << __FUNCTION__ << ";" << __LINE__;
		return hr;
	}
	hr = sfd.fd->SetOptions(dwFlags | _FILEOPENDIALOGOPTIONS::FOS_STRICTFILETYPES);
	if (FAILED(hr))
	{
		std::cout << "ME FAILED AT " << __FUNCTION__ << ";" << __LINE__;
		return hr;
	}
	hr = sfd.fd->SetDefaultExtension(L"pdf");
	if (FAILED(hr))
	{
		std::cout << "ME FAILED AT " << __FUNCTION__ << ";" << __LINE__;
		return hr;
	}
	auto [hook_hr, handler, cookie] = hook_event_handler(sfd.fd);
	if (FAILED(hook_hr))
	{
		std::cout << "ME FAILED AT " << __FUNCTION__ << ";" << __LINE__;
		return hook_hr;
	}
	const auto structuredBindingCannotBeCaptured = cookie;
	CustomRelease unadvise_at_end([structuredBindingCannotBeCaptured, &sfd]()
		{
			sfd.fd->Unadvise(structuredBindingCannotBeCaptured);
		});
	hr = sfd.fd->Show(NULL);
	if (SUCCEEDED(hr))
	{
		DoStuffWithProvidedSaveFile(sfd.fd);
	}
	else
	{
		std::cout << "ME FAILED AT " << __FUNCTION__ << ";" << __LINE__;
		return hr;
	}
	return 0;
}

int my_dlg_impl () {
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (FAILED(hr))
	{
		return 1;
	}
	auto final_result = BasicFileSave();
	CoUninitialize();
	return final_result;
}

// Application entry point
int my_dlg()
{
	HRESULT hr = S_OK;
	while(hr == S_OK)
	{
		hr = my_dlg_impl();
	}
	return hr;
}