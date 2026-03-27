using System;
using System.Diagnostics;
using System.IO;
using System.Threading;
using FlaUI.Core;
using FlaUI.Core.AutomationElements;
using FlaUI.Core.Input;
using FlaUI.Core.WindowsAPI;
using FlaUI.UIA3;

namespace ZadigAutomator
{
    class Program
    {
        static readonly string ZadigPath = @"C:\Users\rinry\Tool\zadig-2.9.exe";
        static readonly string PresetPath = @"C:\Users\rinry\Tool\NULink.preset";

        static void Main(string[] args)
        {
            Console.WriteLine("=== Zadig Nu-Link Driver Installer (FlaUI) ===\n");

            // Check if already running elevated
            bool isElevated = new System.Security.Principal.WindowsPrincipal(
                System.Security.Principal.WindowsIdentity.GetCurrent())
                .IsInRole(System.Security.Principal.WindowsBuiltInRole.Administrator);

            Console.WriteLine($"Administrator: {(isElevated ? "YES" : "NO")}");

            if (!isElevated)
            {
                Console.WriteLine("ERROR: This script must run as Administrator to install drivers.");
                Console.WriteLine("Please run as admin and try again.");
                Environment.Exit(1);
            }

            // Kill any existing Zadig processes
            foreach (var proc in Process.GetProcessesByName("zadig"))
            {
                Console.WriteLine($"Killing existing Zadig (PID={proc.Id})");
                proc.Kill();
                Thread.Sleep(500);
            }

            // Verify files exist
            if (!File.Exists(ZadigPath))
            {
                Console.WriteLine($"ERROR: Zadig not found at {ZadigPath}");
                Environment.Exit(1);
            }
            if (!File.Exists(PresetPath))
            {
                Console.WriteLine($"ERROR: Preset not found at {PresetPath}");
                Environment.Exit(1);
            }

            Console.WriteLine($"\nStarting Zadig (elevation will be requested)...");
            var startInfo = new ProcessStartInfo
            {
                FileName = ZadigPath,
                UseShellExecute = true,
                Verb = "runas"
            };
            var zadigProc = Process.Start(startInfo);

            Console.WriteLine("Waiting for Zadig window...");
            Thread.Sleep(3000); // Wait for window and UAC

            try
            {
                using (var app = FlaUI.Core.Application.AttachOrLaunch(new ProcessStartInfo { FileName = ZadigPath }))
                using (var automation = new UIA3Automation())
                {
                    Console.WriteLine("Waiting for main window...");
                    var mainWindow = app.GetMainWindow(automation, TimeSpan.FromSeconds(15));
                    Console.WriteLine($"Window found: '{mainWindow.Name}'");

                    // Inspect available elements
                    InspectWindow(mainWindow);

                    // Try to load preset via menu
                    Console.WriteLine("\nAttempting to load preset...");

                    // Zadig menu: Device -> Load Preset Device...
                    var deviceMenu = mainWindow.FindFirstDescendant(cf => cf.ByName("Device"));
                    if (deviceMenu != null)
                    {
                        Console.WriteLine("Device menu found, clicking...");
                        try { deviceMenu.AsMenuItem().Click(); } catch { deviceMenu.Click(); }
                        Thread.Sleep(300);

                        var loadPreset = mainWindow.FindFirstDescendant(cf => cf.ByName("Load Preset Device..."));
                        if (loadPreset != null)
                        {
                            Console.WriteLine("Load Preset Device found, clicking...");
                            try { loadPreset.AsMenuItem().Click(); } catch { loadPreset.Click(); }
                            Thread.Sleep(500);

                            // File dialog should appear
                            var dialog = mainWindow.FindFirstDescendant(cf => cf.ByClassName("#32770"));
                            if (dialog != null)
                            {
                                Console.WriteLine("File dialog found!");
                                var fileNameBox = dialog.FindFirstDescendant(cf => cf.ByClassName("Edit"));
                                if (fileNameBox != null)
                                {
                                    Console.WriteLine($"File name box found, typing: {PresetPath}");
                                    try { fileNameBox.AsTextBox().Text = PresetPath; } catch { }
                                    Thread.Sleep(200);
                                    Keyboard.Press(VirtualKeyShort.ENTER);
                                    Thread.Sleep(500);
                                }
                            }
                        }
                        else
                        {
                            Console.WriteLine("Load Preset Device not found, trying Ctrl+O...");
                            Keyboard.Press(VirtualKeyShort.CONTROL);
                            Keyboard.Press(VirtualKeyShort.KEY_O);
                            Keyboard.Release(VirtualKeyShort.CONTROL);
                            Thread.Sleep(500);
                        }
                    }
                    else
                    {
                        Console.WriteLine("Device menu not found");
                    }

                    // Re-inspect
                    Console.WriteLine("\nRe-inspecting window...");
                    Thread.Sleep(1000);
                    InspectWindow(mainWindow);

                    // Look for device list / ComboBox
                    Console.WriteLine("\nLooking for device list (0416:511C)...");
                    var deviceList = mainWindow.FindFirstDescendant(cf => cf.ByClassName("ComboBox"));
                    if (deviceList != null)
                    {
                        Console.WriteLine($"ComboBox found: {deviceList.Name}");
                        try { deviceList.AsComboBox().Click(); Thread.Sleep(500); } catch { }
                    }

                    // Look for WinUSB
                    Console.WriteLine("Looking for WinUSB option...");
                    var winusbItems = mainWindow.FindAllDescendants(cf => cf.ByName("WinUSB"));
                    Console.WriteLine($"Found {winusbItems.Length} WinUSB items");
                    if (winusbItems.Length > 0)
                    {
                        Console.WriteLine("Clicking first WinUSB...");
                        try { winusbItems[0].Click(); } catch { }
                        Thread.Sleep(200);
                    }

                    // Look for Replace Driver / Install Driver button
                    Console.WriteLine("Looking for Replace Driver button...");
                    var replaceButton = mainWindow.FindFirstDescendant(cf => cf.ByName("Replace Driver"));
                    if (replaceButton == null)
                        replaceButton = mainWindow.FindFirstDescendant(cf => cf.ByName("Install Driver"));

                    if (replaceButton != null)
                    {
                        Console.WriteLine($"Found: '{replaceButton.Name}'");
                        Console.WriteLine("\n*** ABOUT TO CLICK REPLACE DRIVER ***");
                        Console.WriteLine("If UAC dialog is showing, click YES first!");
                        Console.WriteLine("Clicking in 5 seconds...");
                        Thread.Sleep(5000);
                        try { replaceButton.AsButton().Click(); } catch { replaceButton.Click(); }
                        Console.WriteLine("Clicked!");
                        Thread.Sleep(3000);
                    }
                    else
                    {
                        Console.WriteLine("Replace Driver button NOT found. Available buttons:");
                        foreach (var btn in mainWindow.FindAllDescendants(cf => cf.ByControlType(FlaUI.Core.Definitions.ControlType.Button)))
                        {
                            Console.WriteLine($"  - '{btn.Name}'");
                        }
                    }

                    Console.WriteLine("\n=== Done. Check Zadig UI for result. ===");
                    Console.WriteLine("Press Enter to exit...");
                    Console.ReadLine();
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"ERROR: {ex.Message}");
                Console.WriteLine(ex.StackTrace);
                Console.ReadLine();
            }
        }

        static void InspectWindow(Window mainWindow)
        {
            Console.WriteLine($"\n--- Window: '{mainWindow.Name}' ---");
            Console.WriteLine($"ClassName: {mainWindow.ClassName}");

            Console.WriteLine("\nTop-level children:");
            foreach (var child in mainWindow.FindAllChildren())
            {
                Console.WriteLine($"  [{child.ControlType}] '{child.Name}' ({child.ClassName})");
            }

            Console.WriteLine("\nAll buttons:");
            foreach (var btn in mainWindow.FindAllDescendants(cf => cf.ByControlType(FlaUI.Core.Definitions.ControlType.Button)))
            {
                Console.WriteLine($"  '{btn.Name}'");
            }

            Console.WriteLine("\nAll ComboBoxes:");
            foreach (var cb in mainWindow.FindAllDescendants(cf => cf.ByControlType(FlaUI.Core.Definitions.ControlType.ComboBox)))
            {
                Console.WriteLine($"  '{cb.Name}'");
            }
        }
    }
}
