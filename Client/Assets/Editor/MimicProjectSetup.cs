using System;
using System.IO;
using Mimic;
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
            PlayerSettings.companyName = "MIMIC";
            PlayerSettings.productName = "MIMIC";
            PlayerSettings.SetApplicationIdentifier(UnityEditor.Build.NamedBuildTarget.Standalone, "com.mimic.holdem");
            PlayerSettings.defaultScreenWidth = 1100;
            PlayerSettings.defaultScreenHeight = 720;
            PlayerSettings.runInBackground = true;
            PlayerSettings.insecureHttpOption = InsecureHttpOption.DevelopmentOnly;
            Directory.CreateDirectory("Assets/_Scenes");
            const string path = "Assets/_Scenes/Bootstrap.unity";
            if (!File.Exists(path))
            {
                var scene = EditorSceneManager.NewScene(NewSceneSetup.EmptyScene, NewSceneMode.Single);
                new GameObject("MIMIC").AddComponent<MimicBootstrap>();
                EditorSceneManager.SaveScene(scene, path);
            }
            EditorBuildSettings.scenes = new[] { new EditorBuildSettingsScene(path, true) };
            AssetDatabase.SaveAssets(); AssetDatabase.Refresh();
            Debug.Log("MIMIC project prepared.");
        }
        [MenuItem("MIMIC/Build Windows development client")]
        public static void BuildWindows()
        {
            Prepare();
            Directory.CreateDirectory("Builds/Windows");
            var report = BuildPipeline.BuildPlayer(new BuildPlayerOptions {
                scenes = new[] { "Assets/_Scenes/Bootstrap.unity" },
                locationPathName = "Builds/Windows/MIMIC.exe", target = BuildTarget.StandaloneWindows64,
                options = BuildOptions.Development
            });
            if (report.summary.result != BuildResult.Succeeded) throw new Exception("MIMIC player build failed");
        }
    }
}
