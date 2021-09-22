using Advanced_Combat_Tracker;
using Microsoft.Win32;
using System;
using System.ComponentModel;
using System.Diagnostics;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Windows.Forms;

[assembly: AssemblyTitle("CutsceneSkip")]
[assembly: AssemblyDescription("Skip Cutscenes in MSQ Roulette")]
[assembly: AssemblyCompany("Bluefissure, modified by winter")]
[assembly: AssemblyVersion("1.0.2.5")]
[assembly: AssemblyCopyright("Copyright © Bluefissure 2021")]

namespace CutsceneSkip
{
    public class PluginEntry : IActPluginV1
    {
        public PluginEntry()
        { }

        private Label m_lbPluginStats;
        private Label m_lbPluginInfo;
        private string m_lbStupidGameProcessInfo;
        private BackgroundWorker m_bgProcessMoniotr;
        private Process m_GameProcess;
        private FFXIV_ACT_Plugin.FFXIV_ACT_Plugin m_ParsingPlugin;

        [DllImport("CutsceneSkipCore.dll")]
        private static extern int initialize(int pid);

        [DllImport("CutsceneSkipCore.dll")]
        private static extern int destroy();

        [DllImport("CutsceneSkipCore.dll")]
        private static extern int on_read_log(string msg, string zone);

        private void OnLogLineRead(bool bIsImport, LogLineEventArgs args)
        {
            int result = on_read_log(args.originalLogLine, args.detectedZone);
            switch (result)
            {
                case -1:
                    m_lbPluginInfo.Text = "Not initialized" + m_lbStupidGameProcessInfo;
                    break;
                case 0:
                    m_lbPluginInfo.Text = "Disabled" + m_lbStupidGameProcessInfo;
                    break;
                case 1:
                    m_lbPluginInfo.Text = "Enabled" + m_lbStupidGameProcessInfo;
                    break;
                case 2:
                    m_lbPluginInfo.Text = "Disabled ( 初见 )" + m_lbStupidGameProcessInfo;
                    break;
            }
        }

        private Process GetGameProcess()
        {
            return m_ParsingPlugin.DataRepository.GetCurrentFFXIVProcess();
        }

        // 从鲶鱼精邮差那儿拿的
        private void ProcessMonitor(object sender, DoWorkEventArgs args)
        {
            while (true)
            {
                if (m_bgProcessMoniotr.CancellationPending)
                {
                    args.Cancel = true;
                    break;
                }

                if (m_GameProcess != GetGameProcess())
                {
                    if (m_GameProcess != null)
                        destroy();

                    m_GameProcess = GetGameProcess();

                    if (m_GameProcess != null)
                    {
                        int result = initialize(m_GameProcess.Id);
                        if (result != 1)
                        {
                            m_lbPluginStats.Text = "Failed --> " + GetErrorMessage(result);
                            args.Cancel = true;
                            throw new Exception("出现错误,到插件列表里查看原因(应该有)");
                        }
                        m_lbStupidGameProcessInfo = "\nProcess ID: " + m_GameProcess.Id + " ( 如果不跳动画请检查Process ID是否与解析插件里的一致 )\n";
                        m_lbPluginInfo.Text = "Initialized" + m_lbStupidGameProcessInfo;
                        m_lbPluginStats.Text = "Working";
                    }
                }
            }
        }

        private string GetErrorMessage(int result)
        {
            switch (result)
            {
                case 0:
                    return "无效handle.";

                case -1:
                    return "找不到地址.";

                case -2:
                    return "权限不够.";
            }

            return "";
        }

        private FFXIV_ACT_Plugin.FFXIV_ACT_Plugin GetParsingPlugin()
        {
            FFXIV_ACT_Plugin.FFXIV_ACT_Plugin entry = null;
            foreach (var actPluginData in ActGlobals.oFormActMain.ActPlugins)
            {
                if (actPluginData.pluginFile.Name.ToUpper().Contains("FFXIV_ACT_Plugin".ToUpper()) && actPluginData.lblPluginStatus.Text.ToUpper().Contains("FFXIV Plugin Started.".ToUpper()))
                {
                    entry = (FFXIV_ACT_Plugin.FFXIV_ACT_Plugin)actPluginData.pluginObj;
                }
            }
            return entry ?? throw new Exception("找不到解析插件FFXIV_ACT_Plugin，请确保插件有安装并启动");
        }

        private bool CheckTheFuckingRedistributableIHatePeopleThatDoesntInstaillItAndITookThisCodeFromStackOverflow()
        {
            string dependenciesPath = @"SOFTWARE\Classes\Installer\Dependencies";

            using (RegistryKey dependencies = Registry.LocalMachine.OpenSubKey(dependenciesPath))
            {
                if (dependencies == null) return false;

                foreach (string subKeyName in dependencies.GetSubKeyNames().Where(n => n.Contains("VC_RuntimeAdditionalVSU_amd64")))
                {
                    using (RegistryKey subDir = Registry.LocalMachine.OpenSubKey(dependenciesPath + "\\" + subKeyName))
                    {
                        var value = subDir.GetValue("DisplayName")?.ToString() ?? null;
                        if (string.IsNullOrEmpty(value)) continue;

                        if (value.Contains("C++ 2019 X64"))
                        {
                            return true;
                        }
                    }
                }
            }

            return false;
        }

        public void InitPlugin(TabPage PluginTab, Label StatusText)
        {
            m_lbPluginStats = StatusText;   // Hand the status label's reference to our local var

            m_ParsingPlugin = GetParsingPlugin();

            // Fuck you C#
            var PluginData = ActGlobals.oFormActMain.PluginGetSelfData(this);
            Environment.SetEnvironmentVariable("PATH", Environment.GetEnvironmentVariable("PATH") + ";" + PluginData.pluginFile.DirectoryName);

            m_bgProcessMoniotr = new BackgroundWorker { WorkerSupportsCancellation = true };
            m_bgProcessMoniotr.DoWork += ProcessMonitor;
            m_bgProcessMoniotr.RunWorkerAsync();

            if (!CheckTheFuckingRedistributableIHatePeopleThatDoesntInstaillItAndITookThisCodeFromStackOverflow())
            {
                throw new Exception("缺少msvc142.dll,需要安装 VC++2019运行库 才能使用插件");
            }

            if (Process.GetProcessesByName("ffxiv_dx11").Length == 0)
            {
                throw new Exception("你要先开游戏再加载插件");
            }

            m_lbPluginInfo = new Label
            {
                AutoSize = true,
                Text = "如果你看到这段文字，那就说明插件加载失败。建议打开ACT日志，拉到最底端，反馈错误日志。\n如果出现「无法加载 DLL“CutsceneSkipCore.dll”: 找不到指定的模块。」这个错误时请确保：\n    1. 有安装 VC++2019 运行库 \n    2. CutsceneSkipCore.dll 与 CutsceneSkip.dll 在同一目录下"
            };
            ActGlobals.oFormActMain.OnLogLineRead += new LogLineEventDelegate(OnLogLineRead);

            PluginTab.Text = "CutsceneSkip";
            PluginTab.Controls.Add(m_lbPluginInfo);

            m_lbPluginStats.Text = "Loaded";

            MessageBox.Show("仅在队伍中没有初见时才会工作。\n免费插件请勿倒卖。\n如果有初见且是自己人时重开插件就能正常使用", "CutsceneSkip");

            m_GameProcess = GetGameProcess() ?? Process.GetProcessesByName("ffxiv_dx11")[0];
            int result = initialize(m_GameProcess.Id);
            if (result != 1)
            {
                throw new Exception(GetErrorMessage(result));
            }

            m_lbStupidGameProcessInfo = "\nProcess ID: " + m_GameProcess.Id + " ( 如果不跳动画请检查Process ID是否与解析插件里的一致 )\n";
            m_lbPluginInfo.Text = "Initialized" + m_lbStupidGameProcessInfo;
        }

        public void DeInitPlugin()
        {
            ActGlobals.oFormActMain.OnLogLineRead -= OnLogLineRead;
            m_bgProcessMoniotr.CancelAsync();

            int result = destroy();

            m_lbPluginStats.Text = "Exited\n" + GetErrorMessage(result);
        }
    }
}
