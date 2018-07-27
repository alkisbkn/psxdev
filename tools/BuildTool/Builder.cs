﻿using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;
using System.Windows.Forms;

namespace BuildTool
{
    /// <summary>
    /// The configuration type
    /// </summary>
    public enum BuildConfiguration
    {
        DebugEMU,
        ReleaseEMU,
        DebugPSX,
        ReleasePSX
    }

    /// <summary>
    /// The license to embed in the generated CD image.
    /// </summary>
    public enum CDLicense
    {
        Europe,
        Americas,
        Japan
    }

    /// <summary>
    /// Encapsulates all clean/build/run functionality for a project.
    /// Supports the following configurations in <see cref="BuildConfiguration"/>    
    /// </summary>
    class Builder
    {
        private string      m_projectDirectory;
        private CDLicense   m_cdLicense;
        private bool        m_generateCD;

        /// <summary>
        /// Create a new builder which will work on the specified directory.
        /// </summary>
        /// <param name="projectDirectory">The project's root directory.</param>
        public Builder(string projectDirectory, CDLicense license, bool generateCD)
        {
            m_projectDirectory = projectDirectory;
            m_cdLicense = license;
            m_generateCD = generateCD;
        }

        /// <summary>
        /// Get the project name (derive it from the project directory).
        /// </summary>
        private string ProjectName
        {
            get
            {
                return Path.GetFileName(m_projectDirectory);
            }
        }

        /// <summary>
        /// Generate an argument string for input to ccpsx.
        /// </summary>
        /// <param name="config">The build configuration.</param>
        /// <param name="additionalPreprocessor">An array of preprocessor names.</param>
        /// <param name="additionalLinker">An array of linker settings.</param>
        /// <returns>The arguments string.</returns>
        string GetCCPSXArgs(BuildConfiguration config, string[] additionalPreprocessor, string[] additionalLinker)
        {
            string args = "-Xo$80010000 ";

            string psxdevRoot = Utilities.GetPsxDevRoot();

            // assemble the arguments
            string engineDirectory = "engine";
            string engineSources = Path.Combine(engineDirectory, "engine_scu.c");
            string gameSources = Path.Combine(m_projectDirectory, "source", "game_scu.c");
            string optimisationLevel = Utilities.IsDebugConfig(config) ? "-O0" : "-O3";
            string[] libraries = { "libpad" };
            string[] libraryDirectories = { };
            string[] includeDirectories = { engineDirectory };

            args += engineSources + " ";
            args += gameSources + " ";
            args += optimisationLevel + " ";

            foreach (string dir in includeDirectories)
            {
                args += "-I" + dir + " ";
            }

            foreach (string dir in libraryDirectories)
            {
                args += "-L" + dir + " ";
            }

            foreach (string lib in libraries)
            {
                args += "-l" + lib + " ";
            }
            
            // Final arguments, output
            args += "-o";
            args += Path.Combine(Utilities.GetBuildOuputDirectory(config, m_projectDirectory), "main.cpe") + ",";
            args += Path.Combine(Utilities.GetBuildOuputDirectory(config, m_projectDirectory), "main.sym") + ",";
            args += Path.Combine(Utilities.GetBuildOuputDirectory(config, m_projectDirectory), "mem.map");

            return args;
        }

        /// <summary>
        /// Clean the generated files for the specified configuration.
        /// </summary>
        /// <param name="config">The build configuration.</param>
        /// <param name="output">The StringBuilder to write out the stdout and stderr streams.</param>
        public void Clean(BuildConfiguration config, StringBuilder output)
        {
            output.Clear();
            output.AppendLine("Cleaning output...");

            string psxdevRoot = Utilities.GetPsxDevRoot();
            string dir = Path.Combine(psxdevRoot, Utilities.GetBuildOuputDirectory(config, m_projectDirectory));
            if (Directory.Exists(dir))
            {
                output.AppendLine("Deleting directory " + dir + "...");
                Directory.Delete(dir, true);
                output.AppendLine("Success!");
            }

            output.AppendLine("Output cleaned");
        }

        /// <summary>
        /// Starts a project build for the specified configuration, adding any extra preprocessor or linker options.
        /// </summary>
        /// <param name="config">The build configuration.</param>
        /// <param name="additionalPreprocessor">An array of preprocessor names.</param>
        /// <param name="additionalLinker">An array of linker settings.</param>
        /// <param name="output">The StringBuilder to write out the stdout and stderr streams.</param>
        public void Build(BuildConfiguration config, string[] additionalPreprocessor, string[] additionalLinker, StringBuilder output)
        {
            output.Clear();
            output.AppendLine("Starting " + config.ToString() + " build...");

            string psxdevRoot = Utilities.GetPsxDevRoot();

            // Create the output directories, if needed
            string outputDir = Path.Combine(psxdevRoot, Utilities.GetBuildOuputDirectory(config, m_projectDirectory));
            if (!Directory.Exists(outputDir))
            {
                output.AppendLine("Output directory doesn't exist, creating " + outputDir + "...");
                Directory.CreateDirectory(outputDir);
                output.AppendLine("Success!");
            }

            // First, the build process.
            {
                // CCPSX
                {
                    output.AppendLine("Launching CCPSX...");
                    string args = GetCCPSXArgs(config, additionalPreprocessor, additionalLinker);
                    Utilities.LaunchProcessAndWait("ccpsx", args, psxdevRoot, output);
                }

                // CPE2X
                {
                    output.AppendLine("Launching CPE2X...");
                    Utilities.LaunchProcessAndWait("cpe2x", "main.cpe", outputDir, output);
                }
            }

            // Then, make the CD image
            // This is useful for NO$PSX, as it seems that's the only way it can load PSX executable at the moment (unless of course we want to burn the image to a CD).
            // First, read the templates (system & cd) and make a copy of them in the output directory. 
            // Modify their placeholder values and add any files to the tracks.
            if (m_generateCD)
            {
                string cdOutputDir = Path.Combine(outputDir, "cdrom");
                Directory.CreateDirectory(cdOutputDir);

                // Generate the cd xml for this project.
                {
                    const string kProjectNameStr = "PROJECTNAME_PLACEHOLDER";
                    const string kPublisherNameStr = "PUBLISHERNAME_PLACEHOLDER";
                    const string kLicenseStr = "LICENSE_PLACEHOLDER";

                    // Pick license from region
                    string licenseFile = "";
                    switch (m_cdLicense)
                    {
                        case CDLicense.Europe:
                            licenseFile = "LICENSEE.DAT";
                            break;
                        case CDLicense.Americas:
                            licenseFile = "LICENSEA.DAT";
                                break;
                        case CDLicense.Japan:
                            licenseFile = "LICENSEJ.DAT";
                            break;
                        default: break;
                    }

                    string licenseSource = Path.Combine(psxdevRoot, "psyq", "cdgen", "LCNSFILE", licenseFile);

                    string cdTemplateContents = File.ReadAllText(Path.Combine(psxdevRoot, "cdrom", "cd_template.xml"));
                    cdTemplateContents = cdTemplateContents.Replace(kProjectNameStr, ProjectName);
                    cdTemplateContents = cdTemplateContents.Replace(kLicenseStr, licenseSource);

                    File.WriteAllText(Path.Combine(cdOutputDir, "cd.xml"), cdTemplateContents);
                }

                // Copy the system file.
                {                    
                    string srcSystemFilename = Path.Combine(psxdevRoot, "cdrom", "system_template.txt");
                    string dstSystemFilename = Path.Combine(cdOutputDir, "system.txt");
                    File.Copy(srcSystemFilename, dstSystemFilename, true);
                }

                // Now we can build the CD image, the MKPSXISO is used for this.
                {
                    output.AppendLine("Launching MKPSXISO...");

                    string args = "cdrom/cd.xml -y";
                    Utilities.LaunchProcessAndWait("mkpsxiso", args, outputDir, output);
                }
            }
        }

        /// <summary>
        /// Starts a project build for the specified configuration, adding any extra preprocessor or linker options. After the build succeeds, the
        /// PSX executable will be launched on either the connected PSX (if any) or the NO$PSX emulator.
        /// In the case of a EMU config, a full CD image will be generated.
        /// </summary>
        /// <param name="config">The build configuration.</param>
        /// <param name="additionaPreprocessor">An array of preprocessor names.</param>
        /// <param name="additionalLinker">An array of linker settings.</param>
        /// <param name="output">The StringBuilder to write out the stdout and stderr streams.</param>
        public void BuildAndRun(BuildConfiguration config, string[] additionaPreprocessor, string[] additionalLinker, StringBuilder output)
        {
            Build(config, additionaPreprocessor, additionalLinker, output);
            Run(config, output);
        }

        /// <summary>
        /// Launches the previously built PSX executableon either the connected PSX (if any) or the NO$PSX emulator.
        /// If there is no output for the specified configuration, the launch will fail, no re-build will be started.
        /// </summary>
        /// <param name="config">The build configuration.</param>
        /// <param name="output">The StringBuilder to write out the stdout and stderr streams.</param>
        public void Run(BuildConfiguration config, StringBuilder output)
        {
            output.Clear();

            string psxdevRoot = Utilities.GetPsxDevRoot();

            string outputDir = Path.Combine(psxdevRoot, Utilities.GetBuildOuputDirectory(config, m_projectDirectory));

            if (Utilities.IsPSXConfig(config))
            {
                // Catflap
                output.AppendLine("Launching on PSX using catflap...");
                Utilities.LaunchProcess("catflap", "run main.exe", outputDir, output);
            }
            else if (Utilities.IsEMUConfig(config))
            {
                // NO$PSX
                output.AppendLine("Launching on NO$PSX emulator...");
                bool success = Utilities.LaunchProcess("no$psx", "cdrom/" + ProjectName + ".cue", outputDir, output);
                output.AppendLine(success ? "Success!" : "Failed (build unavailable?)");
            }
        }
    }
}
