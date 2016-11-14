using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Xml.Linq;
using System.Text.RegularExpressions;
using System.IO;

namespace Codepack
{
    class Program
    {
        static FolderItem[] GetCppFiles(FolderItem folder)
        {
            return Directory
                .GetFiles(folder.Path, "*.cpp", SearchOption.AllDirectories).Concat(
                Directory.GetFiles(folder.Path, "*.c", SearchOption.AllDirectories)
                )
                .Select(s => new FolderItem() { Path = s.ToUpper(), IfNDef = folder.IfNDef})
                .ToArray()
                ;
        }
        static string[] GetHeaderFiles(FolderItem folder)
        {
            return Directory
                .GetFiles(folder.Path, "*.h", SearchOption.AllDirectories).Concat(
                Directory.GetFiles(folder.Path, "*.hpp", SearchOption.AllDirectories)
                )
                .Select(s => s.ToUpper())
                .ToArray()
                ;
        }

        static Dictionary<string, FolderItem[]> CategorizeCodeFiles(XDocument config, FolderItem[] files)
        {
            Dictionary<string, FolderItem[]> categorizedFiles = new Dictionary<string, FolderItem[]>();
            foreach (var e in config.Root.Element("categories").Elements("category"))
            {
                string name = e.Attribute("name").Value;
                string pattern = e.Attribute("pattern").Value.ToUpper();
                string[] exceptions = e.Elements("except").Select(x => x.Attribute("pattern").Value.ToUpper()).ToArray();
                var filteredFiles = files
                        .Where(f =>
                        {
                            string path = f.Path.ToUpper();
                            return path.Contains(pattern) && exceptions.All(ex => !path.Contains(ex));
                        })
                        .ToArray();
                FolderItem[] previousFiles = null;
                if (categorizedFiles.TryGetValue(name, out previousFiles))
                {
                    filteredFiles = filteredFiles.Concat(previousFiles).ToArray();
                    categorizedFiles.Remove(name);
                }
                categorizedFiles.Add(name, filteredFiles);
            }
            foreach (var a in categorizedFiles.Keys)
            {
                foreach (var b in categorizedFiles.Keys)
                {
                    if (a != b)
                    {
                        var intersection = categorizedFiles[a].Intersect(categorizedFiles[b]);
                        if (intersection.Count() != 0)
                        {
                            throw new ArgumentException();
                        }
                    }
                }
            }
            return categorizedFiles;
        }

        static Dictionary<string, string[]> ScannedFiles = new Dictionary<string, string[]>();
        static Regex IncludeRegex = new Regex(@"^\s*\#include\s*""(?<path>[^""]+)""\s*$");
        static Regex IncludeSystemRegex = new Regex(@"^\s*\#include\s*\<(?<path>[^""]+)\>\s*$");

        static string[] GetIncludedFiles(string codeFile)
        {
            codeFile = Path.GetFullPath(codeFile).ToUpper();
            string[] result = null;
            if (!ScannedFiles.TryGetValue(codeFile, out result))
            {
                List<string> directIncludeFiles = new List<string>();
                foreach (var line in File.ReadAllLines(codeFile))
                {
                    Match match = IncludeRegex.Match(line);
                    if (match.Success)
                    {
                        string path = match.Groups["path"].Value;
                        path = Path.GetFullPath(Path.GetDirectoryName(codeFile) + @"\" + path).ToUpper();
                        if (!directIncludeFiles.Contains(path))
                        {
                            directIncludeFiles.Add(path);
                        }
                    }
                }

                for (int i = directIncludeFiles.Count - 1; i >= 0; i--)
                {
                    directIncludeFiles.InsertRange(i, GetIncludedFiles(directIncludeFiles[i]));
                }
                result = directIncludeFiles.Distinct().ToArray();
                ScannedFiles.Add(codeFile, result);
            }
            return result;
        }

        static string[] SortDependecies(Dictionary<string, string[]> dependeicies)
        {
            var dep = dependeicies.ToDictionary(p => p.Key, p => new HashSet<string>(p.Value));
            List<string> sorted = new List<string>();
            while (dep.Count > 0)
            {
                bool found = false;
                foreach (var p in dep)
                {
                    if (p.Value.Count == 0)
                    {
                        found = true;
                        sorted.Add(p.Key);
                        foreach (var q in dep.Values)
                        {
                            q.Remove(p.Key);
                        }
                        dep.Remove(p.Key);
                        break;
                    }
                }
                if (!found)
                {
                    throw new ArgumentException();
                }
            }
            return sorted.ToArray();
        }

        static string GetLongestCommonPrefix(string[] strings)
        {
            if (strings.Length == 0) return "";
            int shortestLength = strings.Select(s => s.Length).Min();
            return Enumerable.Range(0, shortestLength + 1)
                .Reverse()
                .Select(i => strings[0].Substring(0, i))
                .Where(s => strings.Skip(1).All(t => t.StartsWith(s)))
                .First();
        }

        static void Combine(string licenseContent, FolderItem[] files, string outputFilename, HashSet<string> systemIncludes, params string[] externalIncludes)
        {
            try
            {
                string prefix = GetLongestCommonPrefix(files.Select(s => s.Path.ToUpper()).ToArray());
                {
                    int index = prefix.LastIndexOf('/');
                    prefix = prefix.Substring(index + 1);
                }
                using (StreamWriter writer = new StreamWriter(new FileStream(outputFilename, FileMode.Create), new UTF8Encoding(false)))
                {
                    writer.WriteLine("/***********************************************************************");
                    writer.WriteLine(licenseContent);
                    writer.WriteLine();
                    writer.WriteLine("========================================================================");
                    writer.WriteLine("WARNING: THIS FILE IS AUTOMATICALLY GENERATED. DO NOT MODIFY");
                    writer.WriteLine("***********************************************************************/");
                    foreach (var inc in externalIncludes)
                    {
                        writer.WriteLine("#include \"{0}\"", inc);
                    }

                    foreach (var file in files)
                    {
                        writer.WriteLine("");
                        writer.WriteLine("/***********************************************************************");
                        writer.WriteLine(file.Path.Substring(prefix.Length));
                        writer.WriteLine("***********************************************************************/");
                        if (file.IfNDef != null && file.IfNDef != "")
                        {
                            writer.WriteLine("#ifndef " + file.IfNDef);
                        }
                        foreach (var line in File.ReadAllLines(file.Path, Encoding.Default))
                        {
                            Match match = null;

                            match = IncludeSystemRegex.Match(line);
                            if (match.Success)
                            {
                                if (systemIncludes.Add(match.Groups["path"].Value.ToUpper()))
                                {
                                    writer.WriteLine(line);
                                }
                            }
                            else
                            {
                                match = IncludeRegex.Match(line);
                                if (!match.Success)
                                {
                                    writer.WriteLine(line);
                                }
                            }
                        }
                        if (file.IfNDef != null && file.IfNDef != "")
                        {
                            writer.WriteLine("#endif");
                        }
                    }
                }
                Console.WriteLine("Succeeded to write: {0}", outputFilename);
            }
            catch (Exception)
            {
                Console.WriteLine("Failed to write: {0}", outputFilename);
            }
        }

        static void Combine(string licenseContent, string inputFilename, string outputFilename, params string[] externalIncludes)
        {
            HashSet<string> systemIncludes = new HashSet<string>();
            string[] files = GetIncludedFiles(inputFilename).Concat(new string[] { inputFilename }).Distinct().ToArray();
            Combine(licenseContent, files.Select(x=>new FolderItem() { Path = x, IfNDef = "" }).ToArray(), outputFilename, systemIncludes, externalIncludes);
        }
        struct FolderItem
        {
            public string Path;
            public string IfNDef;
        }
        static void Main(string[] args)
        {
            if (args.Length != 1)
            {
                Console.WriteLine("Codepack.exe <config-xml>");
                return;
            }
            // load configuration
            XDocument config = XDocument.Load(args[0]);
            string folder = Path.GetDirectoryName(Path.GetFullPath(args[0])) + "\\";

            string licenseContent = "";
            try
            {
                string licenseFile = config.Root.Element("license").Attribute("name").Value;
                licenseContent = File.ReadAllText(licenseFile);
            }
            catch
            {
            }
            // collect project files
            var folders = config.Root
                .Element("folders")
                .Elements("folder")
                .Select(e => new FolderItem() { Path = Path.GetFullPath(folder + e.Attribute("path").Value), IfNDef = e.Attribute("ifndef")?.Value })
                .ToArray();

            // collect code files
            var unprocessedCppFiles = folders
                .SelectMany(GetCppFiles)
                .Distinct()
                .ToArray();
            string[] unprocessedHeaderFiles = folders
                .SelectMany(GetHeaderFiles)
                .Distinct()
                .ToArray();
            unprocessedHeaderFiles = folders
                .SelectMany(GetHeaderFiles)
                .Concat(unprocessedCppFiles.Select(x=>x.Path))
                .SelectMany(GetIncludedFiles)
                .Concat(unprocessedHeaderFiles)
                .Distinct().ToArray();

            // categorize code files
            var categorizedCppFiles = CategorizeCodeFiles(config, unprocessedCppFiles);
            var categorizedHeaderFiles = CategorizeCodeFiles(config, unprocessedHeaderFiles.Select(x=>new FolderItem() { Path =x, IfNDef=""}).ToArray());
            var outputFolder = Path.GetFullPath(folder + config.Root.Element("output").Attribute("path").Value);
            var categorizedOutput = config.Root
                .Element("output")
                .Elements("codepair")
                .ToDictionary(
                    e => e.Attribute("category").Value,
                    e => Tuple.Create(Path.GetFullPath(outputFolder + "\\" + e.Attribute("filename").Value), bool.Parse(e.Attribute("generate").Value))
                    );

            // calculate category dependencies
            var categoryDependencies = categorizedCppFiles
                .Keys
                .Select(k =>
                {
                    var headerFiles = categorizedCppFiles[k].Select(x=>x.Path)
                        .SelectMany(GetIncludedFiles)
                        .Distinct()
                        .ToArray();
                    var keys = categorizedHeaderFiles
                        .Where(p => p.Value.Any(h => headerFiles.Contains(h.Path)))
                        .Select(p => p.Key)
                        .Except(new string[] { k })
                        .ToArray();
                    return Tuple.Create(k, keys);
                })
                .ToDictionary(t => t.Item1, t => t.Item2);

            // sort categories by dependencies
            var categoryOrder = SortDependecies(categoryDependencies);
            Dictionary<string, HashSet<string>> categorizedSystemIncludes = new Dictionary<string, HashSet<string>>();

            // generate code pair header files
            foreach (var c in categoryOrder)
            {
                string output = categorizedOutput[c].Item1 + ".h";
                List<string> includes = new List<string>();
                foreach (var dep in categoryDependencies[c])
                {
                    includes.AddRange(categorizedSystemIncludes[dep]);
                }
                HashSet<string> systemIncludes = new HashSet<string>(includes.Distinct());
                categorizedSystemIncludes.Add(c, systemIncludes);
                if (categorizedOutput[c].Item2)
                {
                    Combine(
                        licenseContent,
                        categorizedHeaderFiles[c],
                        output,
                        systemIncludes,
                        categoryDependencies[c]
                            .Select(d => Path.GetFileName(categorizedOutput[d].Item1 + ".h"))
                            .ToArray()
                        );
                }
            }

            // generate code pair cpp files
            foreach (var c in categoryOrder)
            {
                if (categorizedOutput[c].Item2)
                {
                    string output = categorizedOutput[c].Item1;
                    string outputHeader = Path.GetFileName(output + ".h");
                    string outputCpp = output + ".cpp";
                    HashSet<string> systemIncludes = categorizedSystemIncludes[c];
                    Combine(
                        licenseContent,
                        categorizedCppFiles[c],
                        outputCpp,
                        systemIncludes,
                        outputHeader
                        );
                }
            }

            // generate header files
            var headerOutput = config.Root
                .Element("output")
                .Elements("header")
                .ToDictionary(
                    e => Path.GetFullPath(folder + e.Attribute("source").Value),
                    e => Path.GetFullPath(outputFolder + e.Attribute("filename").Value)
                    );
            foreach (var o in headerOutput)
            {
                Combine(licenseContent, o.Key, o.Value + ".h");
            }
        }
    }
}