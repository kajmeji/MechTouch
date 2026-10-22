#pragma once
inline bool testUploading=false;
struct UploadManager { static bool isUploading() { return testUploading; } };
