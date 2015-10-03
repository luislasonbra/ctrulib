#include <string.h>
#include <3ds/types.h>
#include <3ds/os.h>
#include <3ds/svc.h>
#include <3ds/srv.h>
#include <3ds/services/news.h>
#include <3ds/ipc.h>

typedef struct {
	bool dataSet;
	bool unread;
	bool enableJPEG;
	u8 unkFlag1;
	u8 unkFlag2;
	u64 processID;
	u8 unkData[24];
	u64 time;
	u16 title[32];
} NotificationHeader;

static Handle newsHandle = 0;

Result newsInit(void) {
	return srvGetServiceHandle(&newsHandle, "news:u");
}

Result newsExit(void) {
	return svcCloseHandle(newsHandle);
}

Result NEWSU_AddNotification(const u16* title, u32 titleLength, const u16* message, u32 messageLength, const void* imageData, u32 imageSize, bool jpeg)
{
	NotificationHeader header = { 0 };
	header.dataSet = true;
	header.unread = true;
	header.enableJPEG = jpeg;
	header.processID = 0; // Filled automatically from FS:GetProgramLaunchInfo
	header.time = osGetTime();
	memcpy(header.title, title, (titleLength < 32 ? titleLength + 1 : 32) * sizeof(u16));

	Result ret = 0;
	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = IPC_MakeHeader(0x1,3,8); // 0x100C8
	cmdbuf[1] = sizeof(NotificationHeader);
	cmdbuf[2] = (messageLength + 1) * sizeof(u16);
	cmdbuf[3] = imageSize;
	cmdbuf[4] = IPC_Desc_CurProcessHandle();
	cmdbuf[5] = 0; // Process ID, Filled automatically by the ARM11 kernel.
	cmdbuf[6] = IPC_Desc_Buffer(sizeof(NotificationHeader),IPC_BUFFER_R);
	cmdbuf[7] = (u32) &header;
	cmdbuf[8] = IPC_Desc_Buffer((messageLength + 1) * sizeof(u16),IPC_BUFFER_R);
	cmdbuf[9] = (u32) message;
	cmdbuf[10] = IPC_Desc_Buffer(imageSize,IPC_BUFFER_R);
	cmdbuf[11] = (u32) imageData;

	if((ret = svcSendSyncRequest(newsHandle))!=0) return ret;

	return (Result)cmdbuf[1];
}
