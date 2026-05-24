#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define FIREHOSE_FILE  "Firehose\\prog_emmc_firehose_8937_ddr.mbn"
#define RAWPROGRAM     "lk2nd\\rawprogram0.xml"
#define EDL_TOOL       "bin\\edl.exe"
#define ADB_TOOL       "bin\\adb.exe"
#define FASTBOOT_TOOL  "bin\\fastboot.exe"

#define MD5_EDL        "33302AF4A273D1D96304ACDF4E82882B"
#define MD5_FIREHOSE   "1626142BED4069F2B9E7F1D14DF86200"
#define MD5_RAWPROGRAM "bac2616127d998a79479e753c800a0fc"
#define MD5_SYSTEM     "acd39d06f0a4e989a89a9f840551ec3f"
#define MD5_VENDOR     "c60a403f1e1cbbeaa90702770133b4ad"
#define MD5_BOOT       "65fbd8e55a508f08c4699db210963148"
#define MD5_RECOVERY   "543afe53dad6baae5a7a5072f0b0a59a"
#define MD5_ABOOT      "76f37ec7d6a006cb76bbce39beb898d5"

#define COLOR_RED      (FOREGROUND_RED | FOREGROUND_INTENSITY)
#define COLOR_GREEN    (FOREGROUND_GREEN | FOREGROUND_INTENSITY)
#define COLOR_DEFAULT  (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)

int CheckFileExist(const char* path);
int CheckMD5Empty();
int CheckMD5(const char* filePath, const char* expectMD5, const char* desc);
int RetryOrExit(const char* errorStage);
int DetectFastboot(int timeoutSec);
int DetectADB(int timeoutSec);
int DetectEDL();
void WaitDevice(int type, const char* name);
void FixWifiAndTime();
void SystemPause();
void ShowLogo();
void ClearCurrentLine();
void SetConsoleColor(WORD color);

void ShowLogo()
{
    printf("\n");
    printf("   ██████╗ ███████╗██████╗ ███████╗███████╗██████╗  ████████╗\n");
    printf("   ██╔══██╗██╔════╝██╔══██╗██╔════╝██╔════╝██╔══██╗╚══██╔══╝\n");
    printf("   ██████╔╝█████╗  ██████╔╝███████╗█████╗  ██████╔╝   ██║   \n");
    printf("   ██╔══██╗██╔══╝  ██╔══██╗╚════██║██╔══╝  ██╔══██╗   ██║   \n");
    printf("   ██║  ██║███████╗██║  ██║███████║███████╗██║  ██║   ██║   \n");
    printf("   ╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝╚══════╝╚══════╝╚═╝  ╚═╝   ╚═╝   \n");
    printf("\n");
    printf("                    OPPO A57 刷机工具                        \n");
    printf("-----------------------------------------------------------\n\n");
}

void SetConsoleColor(WORD color)
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

int DetectEDL()
{
    char buf[1024] = {0};
    FILE* fp = _popen("wmic path Win32_PnPEntity get Name 2>nul", "r");
    if (fp)
    {
        while (fgets(buf, sizeof(buf), fp))
        {
            if (strstr(buf, "Qualcomm HS-USB QDLoader 9008") || 
                strstr(buf, "QDLoader 9008") ||
                strstr(buf, "9008"))
            {
                _pclose(fp);
                return 1;
            }
        }
        _pclose(fp);
    }
    int ret = system(EDL_TOOL" -p >nul 2>&1");
    return (ret == 0);
}

void ClearCurrentLine()
{
    printf("\r");
    for (int i = 0; i < 80; i++) printf(" ");
    printf("\r");
}

void WaitDevice(int type, const char* name)
{
    int lastDriverState = -1;
    int ok = 0;

    system("cls");
    ShowLogo();
    printf("[等待设备] 请将手机进入: %s 模式\n", name);
    printf("-----------------------------------------------------------\n");

    while (1)
    {
        ok = 0;
        int currentDriverState = 0;

        if (type == 0)
        {
            ok = DetectEDL();
            if (!ok)
            {
                FILE* fp = _popen("wmic path Win32_PnPEntity get Name 2>nul", "r");
                if (fp)
                {
                    char buf[1024];
                    while (fgets(buf, sizeof(buf), fp))
                    {
                        if (strstr(buf, "PCI数据捕获和信号处理控制器") || 
                            strstr(buf, "未知设备"))
                        {
                            currentDriverState = 1;
                            break;
                        }
                    }
                    _pclose(fp);
                }
            }
        }
        else if (type == 1) ok = DetectFastboot(1);
        else                ok = DetectADB(1);

        if (ok)
        {
            ClearCurrentLine();
            SetConsoleColor(COLOR_GREEN);
            printf("[状态] %s 设备已连接！", name);
            SetConsoleColor(COLOR_DEFAULT);
            printf("\n");
            printf("-----------------------------------------------------------\n");
            printf("按任意键继续...\n");
            SystemPause();
            break;
        }
        else
        {
            if (type == 0 && currentDriverState != lastDriverState)
            {
                system("cls");
                ShowLogo();
                printf("[等待设备] 请将手机进入: %s 模式\n", name);
                printf("-----------------------------------------------------------\n");
                
                if (currentDriverState)
                {
                    printf("\n  [重要警告] 检测到未安装驱动的9008设备！\n");
                    printf("-----------------------------------------------------------\n");
                    printf("设备已进入9008模式，但高通驱动未正确安装\n");
                    printf("请先安装 Qualcomm HS-USB QDLoader 9008 驱动\n");
                    printf("安装完成后请重新插拔USB线\n");
                    printf("-----------------------------------------------------------\n");
                }
                lastDriverState = currentDriverState;
            }

            ClearCurrentLine();
            SetConsoleColor(COLOR_RED);
            if (type == 0 && currentDriverState)
            {
                printf("[状态] 等待安装驱动并重新插拔设备...");
            }
            else
            {
                printf("[状态] 未检测到 %s 设备，请连接手机并进入对应模式...", name);
            }
            SetConsoleColor(COLOR_DEFAULT);
            fflush(stdout);
            Sleep(1000);
        }
    }
}

int main()
{
    system("chcp 936 >nul");
    SetConsoleTitleA("OPPO A57 刷机工具");
    SetConsoleColor(COLOR_DEFAULT);

    WaitDevice(0, "EDL(9008)");
    system("cls");

    printf("[阶段1/9] 检查MD5配置...\n");
    if (CheckMD5Empty())
    {
        printf("\n  [严重错误] 缺少必要的MD5配置！\n");
        printf("请编辑程序，在MD5校验配置部分填写所有文件的正确MD5值\n");
        SystemPause();
        return 0;
    }
    printf("[阶段1/9]  MD5配置检查完成\n\n");

    printf("[阶段2/9] 检查基础文件存在性...\n");
    if (!CheckFileExist(EDL_TOOL) ||
        !CheckFileExist(FIREHOSE_FILE) ||
        !CheckFileExist(RAWPROGRAM) ||
        !CheckFileExist("images\\system.img") ||
        !CheckFileExist("images\\vendor.img") ||
        !CheckFileExist("images\\boot.img") ||
        !CheckFileExist("images\\recovery.img") ||
        !CheckFileExist("images\\emmc_appsboot.mbn"))
    {
        RetryOrExit("阶段2/9 基础文件检查失败");
        return 0;
    }
    printf("[阶段2/9]  所有基础文件存在\n\n");

    printf("[阶段3/9] 开始强制校验文件MD5完整性...\n");
    printf("  警告：任何文件MD5不匹配都会立即终止脚本！\n\n");
    if (CheckMD5(EDL_TOOL, MD5_EDL, "edl.exe") ||
        CheckMD5(FIREHOSE_FILE, MD5_FIREHOSE, "Firehose文件") ||
        CheckMD5(RAWPROGRAM, MD5_RAWPROGRAM, "rawprogram0.xml") ||
        CheckMD5("images\\system.img", MD5_SYSTEM, "system.img") ||
        CheckMD5("images\\vendor.img", MD5_VENDOR, "vendor.img") ||
        CheckMD5("images\\boot.img", MD5_BOOT, "boot.img") ||
        CheckMD5("images\\recovery.img", MD5_RECOVERY, "recovery.img") ||
        CheckMD5("images\\emmc_appsboot.mbn", MD5_ABOOT, "emmc_appsboot.mbn"))
    {
        RetryOrExit("阶段3/9 MD5校验失败");
        return 0;
    }
    printf("\n================================================\n");
    printf("      【安全通过】所有文件MD5校验完全匹配！\n");
    printf("================================================\n");
    printf("文件完整性确认无误，可以安全进行刷机\n");
    printf("================================================\n");
    printf("按任意键进行下一步\n\n");
    SystemPause();
    system("cls");

FLASH_LK2ND:
printf("\n");
printf("  ██████╗ ███████╗██████╗  █████╗ ██████╗ ███████╗██╗  ██╗███████╗\n");
printf("  ██╔══██╗██╔════╝██╔══██╗██╔══██╗██╔══██╗██╔════╝██║  ██║██╔════╝\n");
printf("  ██████╔╝█████╗  ██████╔╝███████║██████╔╝█████╗  ███████║█████╗  \n");
printf("  ██╔══██╗██╔══╝  ██╔══██╗██╔══██║██╔══██╗██╔══╝  ██╔══██║██╔══╝  \n");
printf("  ██████╔╝███████╗██║  ██║██║  ██║██║  ██║███████╗██║  ██║███████╗\n");
printf("  ╚═════╝ ╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝╚══════╝\n");
printf("\n");
printf("  ██████╗ ██████╗ ███████╗██████╗  █████╗ ██████╗ ███████╗██╗  ██╗\n");
printf("  ██╔══██╗██╔══██╗██╔════╝██╔══██╗██╔══██╗██╔══██╗██╔════╝██║  ██║\n");
printf("  ██████╔╝██████╔╝█████╗  ██████╔╝███████║██████╔╝█████╗  ███████║█████╗  \n");
printf("  ██╔═══╝ ██╔══██╗██╔══╝  ██╔══██╗██╔══██║██╔══██╗██╔══╝  ██╔══██║██╔══╝  \n");
printf("  ██║     ██║  ██║███████╗██║  ██║██║  ██║██║  ██║███████╗██║  ██║███████╗\n");
printf("  ╚═╝     ╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝╚══════╝\n");
printf("\n");
printf("-----------------------------------------------------------\n");
printf("  [阶段4/9] 准备刷入lk2nd...\n");
printf("  按任意键开始刷入lk2nd\n");
printf("-----------------------------------------------------------\n\n");
    SystemPause();
    printf("[阶段4/9] 开始刷入lk2nd...\n");
    char cmd[512];
    sprintf(cmd, "%s --loader %s rawprogram %s", EDL_TOOL, FIREHOSE_FILE, RAWPROGRAM);
    int ret = system(cmd);
    if (ret != 0)
    {
        printf("\n  [错误] lk2nd刷入失败\n");
        if (RetryOrExit("阶段4/9 lk2nd刷入失败"))
            goto FLASH_LK2ND;
        else
            return 0;
    }
    printf("[阶段4/9]  lk2nd刷入成功\n");
    system(EDL_TOOL" reset");
    printf("设备将自动重启，请等待进入fastboot模式...\n\n");

    WaitDevice(1, "Fastboot");
    system("cls");

    printf("[阶段5/9] 检查LineageOS刷机文件...\n");
    printf("(文件已在MD5校验阶段确认存在且完整)\n\n");

    if (!DetectFastboot(30))
    {
        printf("\n  [错误] 未检测到fastboot设备\n");
        printf("请确保设备已进入Fastboot模式并重新连接USB\n");
        SystemPause();
        return 0;
    }

    char confirm[10];
CONFIRM:
    printf("警告：继续操作将清空手机所有数据！\n");
    printf("请输入 YES 确认继续刷机：");
    fgets(confirm, sizeof(confirm), stdin);
    confirm[strcspn(confirm, "\n")] = 0;
    if (_stricmp(confirm, "YES") != 0)
    {
        printf("用户取消操作\n");
        SystemPause();
        return 0;
    }
    printf("\n");

FLASH_IMAGES:
    printf("[阶段7/9] 开始刷入系统镜像...\n");
    ret = system(FASTBOOT_TOOL" flash system images\\system.img");
    if (ret) { printf("\n  [错误] system刷入失败\n"); RetryOrExit("system刷入失败"); goto FLASH_IMAGES; }
    printf("[成功] system镜像刷入完成\n\n");

    ret = system(FASTBOOT_TOOL" flash oem images\\vendor.img");
    if (ret) { printf("\n  [错误] vendor刷入失败\n"); RetryOrExit("vendor刷入失败"); goto FLASH_IMAGES; }
    printf("[成功] vendor镜像刷入完成\n\n");

    ret = system(FASTBOOT_TOOL" flash aboot images\\emmc_appsboot.mbn");
    if (ret) { printf("\n  [错误] aboot刷入失败\n"); RetryOrExit("aboot刷入失败"); goto FLASH_IMAGES; }
    printf("[成功] aboot刷入完成\n\n");

    ret = system(FASTBOOT_TOOL" flash recovery images\\recovery.img");
    if (ret) { printf("\n  [错误] recovery刷入失败\n"); RetryOrExit("recovery刷入失败"); goto FLASH_IMAGES; }
    printf("[成功] recovery镜像刷入完成\n\n");

    ret = system(FASTBOOT_TOOL" flash boot images\\boot.img");
    if (ret) { printf("\n  [错误] boot刷入失败\n"); RetryOrExit("boot刷入失败"); goto FLASH_IMAGES; }
    printf("[成功] boot镜像刷入完成\n\n");

ERASE:
    printf("[阶段8/9] 清除cache分区...\n");
    ret = system(FASTBOOT_TOOL" erase cache");
    if (ret) { printf("\n  [错误] cache清除失败\n"); RetryOrExit("cache清除失败"); goto ERASE; }
    printf("[阶段8/9] 清除userdata分区...\n");
    ret = system(FASTBOOT_TOOL" erase userdata");
    if (ret) { printf("\n  [错误] userdata清除失败\n"); RetryOrExit("userdata清除失败"); goto ERASE; }
    printf("[成功] 分区清理完成\n\n");

    printf("[阶段9/9] 重启设备...\n");
    system(FASTBOOT_TOOL" reboot");
    printf("设备正在重启，等待ADB连接（首次开机可能需要3-5分钟）...\n\n");

    WaitDevice(2, "ADB(USB调试)");
    system("cls");

    FixWifiAndTime();

    printf("\n==========================================\n");
    printf("               全部操作完成\n");
    printf("==========================================\n");
    printf("OPPO A57 （2016） 刷机成功！\n");
    printf("==========================================\n");
    printf("按任意键退出...\n");
    SystemPause();
    return 0;
}

int CheckFileExist(const char* path)
{
    FILE* f = fopen(path, "rb");
    if (f)
    {
        fclose(f);
        return 1;
    }
    printf("  [错误] 未找到文件：%s\n", path);
    return 0;
}

int CheckMD5Empty()
{
    if (!strcmp(MD5_EDL, "") || !strcmp(MD5_FIREHOSE, "") || !strcmp(MD5_RAWPROGRAM, "") ||
        !strcmp(MD5_SYSTEM, "") || !strcmp(MD5_VENDOR, "") || !strcmp(MD5_BOOT, "") ||
        !strcmp(MD5_RECOVERY, "") || !strcmp(MD5_ABOOT, ""))
    {
        return 1;
    }
    return 0;
}

int CheckMD5(const char* filePath, const char* expectMD5, const char* desc)
{
    if (!_stricmp(desc, "system.img"))
        printf("[提示] 正在校验 %s (较大，请耐心等待)...\n", desc);
    else
        printf("[提示] 正在校验 %s...\n", desc);
    char cmd[512];
    sprintf(cmd, "CertUtil -hashfile \"%s\" MD5 2>&1", filePath);
    FILE* fp = _popen(cmd, "r");
    if (!fp)
    {
        printf("  [错误] 无法执行MD5校验命令\n");
        return 1;
    }
    char line[1024];
    char realMD5[64] = {0};
    int md5Len = 0;
    if (fgets(line, sizeof(line), fp) == NULL)
    {
        _pclose(fp);
        printf("  [错误] 读取MD5输出失败\n");
        return 1;
    }
    if (fgets(line, sizeof(line), fp) == NULL)
    {
        _pclose(fp);
        printf("  [错误] 未找到MD5值行\n");
        return 1;
    }
    for (int i = 0; line[i] && md5Len < 32; i++)
    {
        if (isxdigit((unsigned char)line[i]))
        {
            realMD5[md5Len++] = tolower((unsigned char)line[i]);
        }
    }
    _pclose(fp);
    if (md5Len != 32)
    {
        printf("  [错误] 无法获取 %s 的有效MD5值\n", desc);
        return 1;
    }
    if (!_stricmp(realMD5, expectMD5))
    {
        printf("  [成功] %s MD5校验通过\n", desc);
        return 0;
    }
    else
    {
        printf("  [失败] %s MD5不匹配\n", desc);
        printf("  预期：%s\n  实际：%s\n", expectMD5, realMD5);
        return 1;
    }
}

int RetryOrExit(const char* errorStage)
{
    printf("\n====================== 错误处理 ======================\n");
    printf("失败阶段：%s\n", errorStage);
    printf("请选择操作 [1=重新尝试 2=退出脚本]：");
    char c[10];
    fgets(c, sizeof(c), stdin);
    c[strcspn(c, "\n")] = 0;
    if (c[0] == '1') return 1;
    return 0;
}

int DetectFastboot(int timeoutSec)
{
    int cnt = 0;
    while (cnt < timeoutSec)
    {
        if (!system(FASTBOOT_TOOL" devices | findstr \"fastboot\" >nul"))
            return 1;
        Sleep(1000);
        cnt++;
    }
    return 0;
}

int DetectADB(int timeoutSec)
{
    int cnt = 0;
    while (cnt < timeoutSec)
    {
        if (!system(ADB_TOOL" devices | findstr /r \"device$\" >nul"))
            return 1;
        Sleep(1000);
        cnt++;
    }
    return 0;
}

void FixWifiAndTime()
{
    printf("[后续] 开始修复WiFi小叉和时间同步问题...\n\n");
    system(ADB_TOOL" start-server >nul 2>&1");
    printf("清除原有网络验证配置...\n");
    system(ADB_TOOL" shell settings delete global captive_portal_https_url >nul 2>&1");
    system(ADB_TOOL" shell settings delete global captive_portal_http_url >nul 2>&1");
    printf("设置小米网络验证服务器...\n");
    system(ADB_TOOL" shell settings put global captive_portal_http_url http://connect.rom.miui.com/generate_204");
    system(ADB_TOOL" shell settings put global captive_portal_https_url https://connect.rom.miui.com/generate_204");
    printf("设置阿里云NTP时间服务器...\n");
    system(ADB_TOOL" shell settings put global ntp_server ntp1.aliyun.com");
    system(ADB_TOOL" shell settings put global auto_time 1");
    system(ADB_TOOL" shell settings put global auto_time_zone 1");
    printf("\n  [完成] WiFi与时间修复完毕\n");
}

void SystemPause()
{
    system("pause >nul");
}
