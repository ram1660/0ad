/* Copyright (C) 2018 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"
#include "lib/external_libraries/enet.h"
#include "JSInterface_VFS.h"

#include "lib/file/vfs/vfs_util.h"
#include "ps/CLogger.h"
#include "ps/CStr.h"
#include "ps/Filesystem.h"
#include "scriptinterface/ScriptVal.h"
#include "scriptinterface/ScriptInterface.h"
#include <sstream>

// Only allow engine compartments to read files they may be concerned about.
#define PathRestriction_GUI {L""}
#define PathRestriction_Simulation {L"simulation/"}
#define PathRestriction_Maps {L"simulation/", L"maps/"}

// shared error handling code
#define JS_CHECK_FILE_ERR(err)\
	/* this is liable to happen often, so don't complain */\
	if (err == ERR::VFS_FILE_NOT_FOUND)\
	{\
		return 0; \
	}\
	/* unknown failure. We output an error message. */\
	else if (err < 0)\
		LOGERROR("Unknown failure in VFS %i", err );
	/* else: success */
WriteBuffer m_buffer;
ENetHost* client;
ENetPeer *peer;
ENetAddress address;
ENetEvent event;
int eventStatus = 1;
// state held across multiple BuildDirEntListCB calls; init by BuildDirEntList.
struct BuildDirEntListState
{
	JSContext* cx;
	JS::PersistentRootedObject filename_array;
	int cur_idx;

	BuildDirEntListState(JSContext* cx_)
		: cx(cx_),
		filename_array(cx, JS_NewArrayObject(cx, JS::HandleValueArray::empty())),
		cur_idx(0)
	{
	}
};
// called for each matching directory entry; add its full pathname to array.
static Status BuildDirEntListCB(const VfsPath& pathname, const CFileInfo& UNUSED(fileINfo), uintptr_t cbData)
{
	BuildDirEntListState* s = (BuildDirEntListState*)cbData;
	JSAutoRequest rq(s->cx);

	JS::RootedObject filenameArrayObj(s->cx, s->filename_array);
	JS::RootedValue val(s->cx);
	ScriptInterface::ToJSVal( s->cx, &val, CStrW(pathname.string()) );
	JS_SetElement(s->cx, filenameArrayObj, s->cur_idx++, val);
	return INFO::OK;
}


// Return an array of pathname strings, one for each matching entry in the
// specified directory.
//   filter_string: default "" matches everything; otherwise, see vfs_next_dirent.
//   recurse: should subdirectories be included in the search? default false.
JS::Value JSI_VFS::BuildDirEntList(ScriptInterface::CxPrivate* pCxPrivate, const std::vector<CStrW>& validPaths, const std::wstring& path, const std::wstring& filterStr, bool recurse)
{
	if (!PathRestrictionMet(pCxPrivate, validPaths, path))
		return JS::NullValue();

	// convert to const wchar_t*; if there's no filter, pass 0 for speed
	// (interpreted as: "accept all files without comparing").
	const wchar_t* filter = 0;
	if (!filterStr.empty())
		filter = filterStr.c_str();

	int flags = recurse ? vfs::DIR_RECURSIVE : 0;

	JSContext* cx = pCxPrivate->pScriptInterface->GetContext();
	JSAutoRequest rq(cx);

	// build array in the callback function
	BuildDirEntListState state(cx);
	vfs::ForEachFile(g_VFS, path, BuildDirEntListCB, (uintptr_t)&state, filter, flags);

	return OBJECT_TO_JSVAL(state.filename_array);
}

// Return true iff the file exits
bool JSI_VFS::FileExists(ScriptInterface::CxPrivate* pCxPrivate, const std::vector<CStrW>& validPaths, const CStrW& filename)
{
	static int counter = 0;
	counter++;
	return PathRestrictionMet(pCxPrivate, validPaths, filename) && g_VFS->GetFileInfo(filename, 0) == INFO::OK;
}

// Return time [seconds since 1970] of the last modification to the specified file.
double JSI_VFS::GetFileMTime(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::wstring& filename)
{
	CFileInfo fileInfo;
	Status err = g_VFS->GetFileInfo(filename, &fileInfo);
	JS_CHECK_FILE_ERR(err);

	return (double)fileInfo.MTime();
}

// Return current size of file.
unsigned int JSI_VFS::GetFileSize(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::wstring& filename)
{
	CFileInfo fileInfo;
	Status err = g_VFS->GetFileInfo(filename, &fileInfo);
	JS_CHECK_FILE_ERR(err);

	return (unsigned int)fileInfo.Size();
}

// Return file contents in a string. Assume file is UTF-8 encoded text.
JS::Value JSI_VFS::ReadFile(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& filename)
{
	JSContext* cx = pCxPrivate->pScriptInterface->GetContext();
	JSAutoRequest rq(cx);

	CVFSFile file;
	if (file.Load(g_VFS, filename) != PSRETURN_OK)
		return JS::NullValue();

	CStr contents = file.DecodeUTF8(); // assume it's UTF-8

	// Fix CRLF line endings. (This function will only ever be used on text files.)
	contents.Replace("\r\n", "\n");

	// Decode as UTF-8
	JS::RootedValue ret(cx);
	ScriptInterface::ToJSVal(cx, &ret, contents.FromUTF8());
	return ret;
}

std::string JSI_VFS::ReadNewFile(ScriptInterface::CxPrivate * pCxPrivate, const std::wstring & filename)
{
	std::ifstream file(filename);
	std::string str = "";
	if (file.is_open())
	{
		std::getline(file, str);

	}
	file.close();
	return str;
}

// Return file contents as an array of lines. Assume file is UTF-8 encoded text.
JS::Value JSI_VFS::ReadFileLines(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& filename)
{
	JSContext* cx = pCxPrivate->pScriptInterface->GetContext();
	JSAutoRequest rq(cx);

	CVFSFile file;
	if (file.Load(g_VFS, filename) != PSRETURN_OK)
		return JSVAL_NULL;

	CStr contents = file.DecodeUTF8(); // assume it's UTF-8

	// Fix CRLF line endings. (This function will only ever be used on text files.)
	contents.Replace("\r\n", "\n");

	// split into array of strings (one per line)
	std::stringstream ss(contents);
	JS::RootedObject line_array(cx, JS_NewArrayObject(cx, JS::HandleValueArray::empty()));
	std::string line;
	int cur_line = 0;

	while (std::getline(ss, line))
	{
		// Decode each line as UTF-8
		JS::RootedValue val(cx);
		ScriptInterface::ToJSVal(cx, &val, CStr(line).FromUTF8());
		JS_SetElement(cx, line_array, cur_line++, val);
	}

	return JS::ObjectValue(*line_array);
}

JS::Value JSI_VFS::ReadJSONFile(ScriptInterface::CxPrivate* pCxPrivate, const std::vector<CStrW>& validPaths, const CStrW& filePath)
{
	if (!PathRestrictionMet(pCxPrivate, validPaths, filePath))
		return JS::NullValue();

	JSContext* cx = pCxPrivate->pScriptInterface->GetContext();
	JSAutoRequest rq(cx);
	JS::RootedValue out(cx);
	pCxPrivate->pScriptInterface->ReadJSONFile(filePath, &out);
	return out;
}

void JSI_VFS::WriteJSONFile(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& filePath, JS::HandleValue val1)
{
	JSContext* cx = pCxPrivate->pScriptInterface->GetContext();
	JSAutoRequest rq(cx);
	// TODO: This is a workaround because we need to pass a MutableHandle to StringifyJSON.
	JS::RootedValue val(cx, val1);

	std::string str(pCxPrivate->pScriptInterface->StringifyJSON(&val, false));

	VfsPath path(filePath);
	WriteBuffer buf;
	buf.Append(str.c_str(), str.length());
	g_VFS->CreateFile(path, buf.Data(), buf.Size());
}

void JSI_VFS::newWriteJSONFile(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& filePath, JS::HandleValue val1)
{
	JSContext* cx = pCxPrivate->pScriptInterface->GetContext();
	JSAutoRequest rq(cx);
	std::ofstream jsonFile(filePath);

	// TODO: This is a workaround because we need to pass a MutableHandle to StringifyJSON.
	JS::RootedValue val(cx, val1);

	std::string str(pCxPrivate->pScriptInterface->StringifyJSON(&val, false));

	VfsPath path(filePath);
	WriteBuffer buf;
	buf.Append(str.c_str(), str.length());
	jsonFile << str;
	jsonFile.close();
	g_VFS->CreateFile(path, buf.Data(), buf.Size());
}

void JSI_VFS::WriteToFile(ScriptInterface::CxPrivate * pCxPrivate, const std::wstring & filePath)
{
	JSContext* cx = pCxPrivate->pScriptInterface->GetContext();
	JSAutoRequest rq(cx);
	//JS::RootedValue val(cx, val1);
	VfsPath path(filePath);
	g_VFS->CreateFile(path, m_buffer.Data(), m_buffer.Size());
}

void JSI_VFS::AppendToBuffer(ScriptInterface::CxPrivate * pCxPrivate, JS::HandleValue val1)
{
	JSContext* cx = pCxPrivate->pScriptInterface->GetContext();
	JSAutoRequest rq(cx);
	JS::RootedValue val(cx, val1);
	// Clearing the "(new string("{important data}"))" - Because we need to enter only the important data.
	std::string str(pCxPrivate->pScriptInterface->ToString(&val, false));
	std::string clearedString;
	size_t foundFisrt = str.find("\"") + 1;
	clearedString = str.substr(foundFisrt);
	size_t foundSecond = clearedString.find("\"");
	clearedString = clearedString.substr(0, foundSecond);
	clearedString.append("\n");
	m_buffer.Append(clearedString.c_str(), clearedString.length());
}


std::string JSI_VFS::SendDataToML(ScriptInterface::CxPrivate * pCxPrivate, JS::HandleValue val1)
{
	JSContext* cx = pCxPrivate->pScriptInterface->GetContext();
	JSAutoRequest rq(cx);
	JS::RootedValue val(cx, val1);
	std::string str(pCxPrivate->pScriptInterface->StringifyJSON(&val, false));
	/* Create a reliable packet of size 7 containing "packet\0" */
	ENetPacket * packet = enet_packet_create(str.c_str(), str.length() + 1, ENET_PACKET_FLAG_RELIABLE);
	/* One could also broadcast the packet by         */
	/* enet_host_broadcast (host, 0, packet);         */
	enet_peer_send(peer, 0, packet);
	//send(sock, str.c_str(), str.length, 0);
	LOGMESSAGE("Send units info... waiting for response.");
	while (true)
	{
		eventStatus = enet_host_service(client, &event, 50000);
		if (eventStatus > 0)
		{
			switch (event.type)
			{
			case ENET_EVENT_TYPE_RECEIVE:
				enet_uint8 answer = *(event.packet->data);
				enet_packet_destroy(event.packet);
				return std::string((char*) answer);
			}
		}
		continue;
	}
}

void JSI_VFS::ConnectToServer(ScriptInterface::CxPrivate * pCxPrivate, const int port)
{
	if (enet_initialize() != 0)
	{
		LOGMESSAGE("An error occured while initializing ENet.");
		return;
	}
	client = enet_host_create(NULL, 1, 2, 0, 0);
	if (client == NULL)
		LOGMESSAGE("ML Server error: Couldn't create client instance!");

	enet_address_set_host(&address, "localhost");
	address.port = 1234;
	peer = enet_host_connect(client, &address, 2, 0);
	if (peer == NULL)
	{
		LOGMESSAGE("Couldn't find the server!");
		return;
	}
	/* Wait up to 1 seconds for the connection attempt to succeed. */
	if (enet_host_service(client, &event, 1000) > 0 &&
		event.type == ENET_EVENT_TYPE_CONNECT)
	{
		LOGMESSAGE("Connection to to ML server succeeded.");
	}
	else
	{
		/* Either the 5 seconds are up or a disconnect event was */
		/* received. Reset the peer in the event the 5 seconds   */
		/* had run out without any significant event.            */
		enet_peer_reset(peer);
		LOGMESSAGE("Connection to ML server failed.");
	}
}

inline bool JSI_VFS::isFileInUse(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& filePath)
{
	struct stat buffer;
	return (stat((char*)filePath.c_str(), &buffer) == 0);
}

bool JSI_VFS::PathRestrictionMet(ScriptInterface::CxPrivate* pCxPrivate, const std::vector<CStrW>& validPaths, const CStrW& filePath)
{
	for (const CStrW& validPath : validPaths)
		if (filePath.find(validPath) == 0)
			return true;

	CStrW allowedPaths;
	for (std::size_t i = 0; i < validPaths.size(); ++i)
	{
		if (i != 0)
			allowedPaths += L", ";

		allowedPaths += L"\"" + validPaths[i] + L"\"";
	}

	JS_ReportError(
		pCxPrivate->pScriptInterface->GetContext(),
		"This part of the engine may only read from %s!",
		utf8_from_wstring(allowedPaths).c_str());

	return false;
}

#define VFS_ScriptFunctions(context)\
JS::Value Script_ReadJSONFile_##context(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& filePath)\
{\
	return JSI_VFS::ReadJSONFile(pCxPrivate, PathRestriction_##context, filePath);\
}\
JS::Value Script_ListDirectoryFiles_##context(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& path, const std::wstring& filterStr, bool recurse)\
{\
	return JSI_VFS::BuildDirEntList(pCxPrivate, PathRestriction_##context, path, filterStr, recurse);\
}\
bool Script_FileExists_##context(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& filePath)\
{\
	return JSI_VFS::FileExists(pCxPrivate, PathRestriction_##context, filePath);\
}\

VFS_ScriptFunctions(GUI);
VFS_ScriptFunctions(Simulation);
VFS_ScriptFunctions(Maps);
#undef VFS_ScriptFunctions

void JSI_VFS::RegisterScriptFunctions_GUI(const ScriptInterface& scriptInterface)
{
	scriptInterface.RegisterFunction<JS::Value, std::wstring, std::wstring, bool, &Script_ListDirectoryFiles_GUI>("ListDirectoryFiles");
	scriptInterface.RegisterFunction<bool, std::wstring, Script_FileExists_GUI>("FileExists");
	scriptInterface.RegisterFunction<double, std::wstring, &JSI_VFS::GetFileMTime>("GetFileMTime");
	scriptInterface.RegisterFunction<unsigned int, std::wstring, &JSI_VFS::GetFileSize>("GetFileSize");
	scriptInterface.RegisterFunction<JS::Value, std::wstring, &JSI_VFS::ReadFile>("ReadFile");
	scriptInterface.RegisterFunction<JS::Value, std::wstring, &JSI_VFS::ReadFileLines>("ReadFileLines");
	scriptInterface.RegisterFunction<JS::Value, std::wstring, &Script_ReadJSONFile_GUI>("ReadJSONFile");
	scriptInterface.RegisterFunction<void, std::wstring, JS::HandleValue, &WriteJSONFile>("WriteJSONFile");
}

void JSI_VFS::RegisterScriptFunctions_Simulation(const ScriptInterface& scriptInterface)
{
	//scriptInterface.RegisterFunction<JS::Value, JS::HandleValue, &SendDataToML>("sendDataToMLServer");
	//scriptInterface.RegisterFunction<void, &ConnectToServer>("connectML");
	scriptInterface.RegisterFunction<JS::Value, std::wstring, std::wstring, bool, &Script_ListDirectoryFiles_Simulation>("ListDirectoryFiles");
	scriptInterface.RegisterFunction<bool, std::wstring, Script_FileExists_Simulation>("FileExists");
	scriptInterface.RegisterFunction<JS::Value, std::wstring, &Script_ReadJSONFile_Simulation>("ReadJSONFile");
	scriptInterface.RegisterFunction<void, std::wstring, &WriteToFile>("WriteToFile");
	scriptInterface.RegisterFunction<void, JS::HandleValue, &AppendToBuffer>("AppendToBuffer");
	scriptInterface.RegisterFunction<JS::Value, std::wstring, &JSI_VFS::ReadFile>("ReadFile");
	scriptInterface.RegisterFunction<JS::Value, std::wstring, &JSI_VFS::ReadFileLines>("ReadFileLines");
	scriptInterface.RegisterFunction<void, std::wstring, JS::HandleValue, &WriteJSONFile>("WriteJSONFile");
	scriptInterface.RegisterFunction<bool, std::wstring, &JSI_VFS::isFileInUse>("isFileInUse");
	scriptInterface.RegisterFunction<std::string, std::wstring, &JSI_VFS::ReadNewFile>("ReadNewFile");

}

void JSI_VFS::RegisterScriptFunctions_Maps(const ScriptInterface& scriptInterface)
{
	scriptInterface.RegisterFunction<JS::Value, std::wstring, std::wstring, bool, &Script_ListDirectoryFiles_Maps>("ListDirectoryFiles");
	scriptInterface.RegisterFunction<bool, std::wstring, Script_FileExists_Maps>("FileExists");
	scriptInterface.RegisterFunction<void, std::wstring, JS::HandleValue, &WriteJSONFile>("WriteJSONFile");
	scriptInterface.RegisterFunction<JS::Value, std::wstring, &Script_ReadJSONFile_Maps>("ReadJSONFile");
	scriptInterface.RegisterFunction<void, std::wstring, &WriteToFile>("WriteToFile");
	scriptInterface.RegisterFunction<void, JS::HandleValue, &AppendToBuffer>("AppendToBuffer");
	scriptInterface.RegisterFunction<JS::Value, std::wstring, &JSI_VFS::ReadFile>("ReadFile");
}
