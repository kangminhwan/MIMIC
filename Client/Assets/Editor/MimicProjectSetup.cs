using System;
using System.IO;
using UnityEditor;
using UnityEditor.Build.Reporting;
using UnityEditor.SceneManagement;
using UnityEngine;
namespace Mimic.Editor
{
    public static class MimicProjectSetup
    {
        [MenuItem("MIMIC/Prepare project")]
        public static void Prepare()
        {
            foreach (string scene in MimicSceneBuilder.Scenes)
                if (!File.Exists(scene)) { MimicSceneBuilder.Generate(); return; }
            var scenes = new EditorBuildSettingsScene[MimicSceneBuilder.Scenes.Length];
            for (int i=0;i<scenes.Length;++i) scenes[i] = new EditorBuildSettingsScene(MimicSceneBuilder.Scenes[i], true);
            EditorBuildSettings.scenes = scenes; AssetDatabase.SaveAssets();
        }
        [MenuItem("MIMIC/Open starting scene")]
        public static void OpenTitle()
        {
            Prepare();
            if (EditorSceneManager.SaveCurrentModifiedScenesIfUserWantsTo()) EditorSceneManager.OpenScene(MimicSceneBuilder.Scenes[0]);
        }
        [MenuItem("MIMIC/Build Windows development client")]
        public static void BuildWindows()
        {
            Prepare(); Directory.CreateDirectory("Builds/Windows");
            var report = BuildPipeline.BuildPlayer(new BuildPlayerOptions {
                scenes = MimicSceneBuilder.Scenes,
                locationPathName = "Builds/Windows/MIMIC.exe", target = BuildTarget.StandaloneWindows64,
                options = BuildOptions.Development
            });
            if (report.summary.result != BuildResult.Succeeded) throw new Exception("MIMIC player build failed");
        }
        public static void GenerateAndBuild() { MimicSceneBuilder.Generate(); BuildWindows(); }
    }
}
