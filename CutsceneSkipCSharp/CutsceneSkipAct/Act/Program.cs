using System;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Forms;
using Advanced_Combat_Tracker;
using System.ComponentModel;
using FFXIV_ACT_Plugin;
using FFXIV_ACT_Plugin.Common;
using FFXIV_ACT_Plugin.Common.Models;
using System.Reflection;

[assembly: AssemblyTitle("CutsceneSkip")]
[assembly: AssemblyDescription("Skip Cutscenes in MSQ Roulette")]
[assembly: AssemblyCompany("Bluefissure, modified by winter")]
[assembly: AssemblyVersion("1.0.2.2")]
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
                case 0:
                    m_lbPluginInfo.Text = "Disabled" + m_lbStupidGameProcessInfo;
                    break;
                case 1:
                    m_lbPluginInfo.Text = "Enabled" + m_lbStupidGameProcessInfo;
                    break;
                case 2:
                    m_lbPluginInfo.Text = "Disabled ( 初见 )" + m_lbStupidGameProcessInfo;
                    MessageBox.Show("队伍里有初见，插件已关闭。如果是自己人重开插件即可正常使用", "CutsceneSkip");
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
                            throw new Exception(GetErrorMessage(result));
                        }
                        m_lbStupidGameProcessInfo = "\nProcess ID: " + m_GameProcess.Id + " ( 如果不跳动画请检查Process ID是否与解析插件里的一致 )\n";
                        m_lbPluginInfo.Text = "Initialized" + m_lbStupidGameProcessInfo;
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
                    return "重要的东西被摸你傻偷走了.";

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

            m_lbPluginStats.Text = "Working";

            MessageBox.Show("仅在队伍中没有初见时才会工作。\n免费插件请勿倒卖。\n如果有初见且是自己人时重开插件就能正常使用", "CutsceneSkip");
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
