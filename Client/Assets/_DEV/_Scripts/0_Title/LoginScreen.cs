using System;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using Mimic.UI;
using UnityEngine;
using UnityEngine.UI;
namespace Mimic.Screens
{
    public sealed class LoginScreen : ScreenBase
    {
        public InputField account, password, confirmPassword, nickname;
        public Button loginTab, registerTab, submit, back;
        public Toggle rememberAccount, showPassword;
        public GameObject registerFields;
        public Text heading, subheading, submitLabel, validation;
        public RectTransform optionsRow, submitRow, validationRect;
        private bool registering;
        public bool IsRegistering => registering;
        private void Start()
        {
            account.text = PlayerPrefs.GetString("MIMIC.RememberedAccount", "");
            rememberAccount.isOn = account.text.Length > 0;
            loginTab.onClick.AddListener(() => SetMode(false)); registerTab.onClick.AddListener(() => SetMode(true));
            submit.onClick.AddListener(Submit); back.onClick.AddListener(() => Game.Run(Game.ShowTitle));
            showPassword.onValueChanged.AddListener(visible => { password.contentType = visible ? InputField.ContentType.Standard : InputField.ContentType.Password; password.ForceLabelUpdate(); });
            account.onValueChanged.AddListener(_ => ClearValidation()); password.onValueChanged.AddListener(_ => ClearValidation());
            confirmPassword.onValueChanged.AddListener(_ => ClearValidation()); nickname.onValueChanged.AddListener(_ => ClearValidation());
            SetMode(false);
        }
        public void SetMode(bool register)
        {
            if (Game != null && Game.Busy) return;
            registering = register; registerFields.SetActive(register);
            optionsRow.anchoredPosition = new Vector2(0, register ? -697 : -470);
            validationRect.anchoredPosition = new Vector2(0, register ? -748 : -520);
            submitRow.anchoredPosition = new Vector2(0, register ? -800 : -580);
            heading.text = register ? "새로운 플레이어를 환영합니다" : "다시 만나 반가워요";
            subheading.text = register ? "MIMIC 계정을 만들고 첫 핸드를 시작하세요." : "계정으로 로그인하고 테이블에 합류하세요.";
            submitLabel.text = register ? "계정 만들고 시작하기" : "로그인";
            loginTab.interactable = register; registerTab.interactable = !register;
            password.text = ""; confirmPassword.text = ""; validation.text = "";
        }
        private void ClearValidation() { validation.text = ""; }
        private void Update()
        {
            if (Input.GetKeyDown(KeyCode.Return) && !Game.Busy && (account.isFocused || password.isFocused || confirmPassword.isFocused || nickname.isFocused)) Submit();
        }
        public void Submit()
        {
            if (Game.Busy) return;
            string name = account.text.Trim(), secret = password.text;
            if (!Regex.IsMatch(name, "^[A-Za-z0-9_]{3,24}$")) { validation.text = "아이디는 영문·숫자·밑줄 3~24자로 입력해 주세요."; return; }
            if (secret.Length < 8 || secret.Length > 128) { validation.text = "비밀번호는 8~128자로 입력해 주세요."; return; }
            string display = nickname.text.Trim();
            if (registering && (display.Length < 2 || display.Length > 24 || HasControl(display))) { validation.text = "닉네임은 2~24자로 입력해 주세요."; return; }
            if (registering && secret != confirmPassword.text) { validation.text = "비밀번호 확인이 일치하지 않습니다."; return; }
            bool register = registering;
            if (rememberAccount.isOn) PlayerPrefs.SetString("MIMIC.RememberedAccount", name); else PlayerPrefs.DeleteKey("MIMIC.RememberedAccount");
            PlayerPrefs.Save();
            Game.Run(async () =>
            {
                try { if (register) await Game.Register(name, secret, display); else await Game.LoginAccount(name, secret); }
                finally { if (this != null) { password.text = ""; confirmPassword.text = ""; } }
            });
        }
        private static bool HasControl(string value) { foreach (char c in value) if (char.IsControl(c)) return true; return false; }
        protected override void Refresh()
        {
            base.Refresh(); if (Game == null || submit == null) return;
            submit.interactable = !Game.Busy; back.interactable = !Game.Busy;
            account.interactable = password.interactable = confirmPassword.interactable = nickname.interactable = !Game.Busy;
        }
    }
}
