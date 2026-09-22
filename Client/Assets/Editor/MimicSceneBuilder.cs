using System.IO;
using Mimic.Screens;
using Mimic.UI;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;
namespace Mimic.Editor
{
    public static class MimicSceneBuilder
    {
        private static readonly Color Bg = Hex("07131E"), PanelBg = Hex("102432"), Light = Hex("19323D"), Gold = Hex("DCBD7A"), White = Hex("F3F2E9"), Muted = Hex("88A2AA"), Teal = Hex("276064");
        private static Font font;
        private static TableCardView tablePrefab;
        private static HoldemSeatView seatPrefab;
        public static readonly string[] Scenes = { "Assets/_Scenes/0_Title.unity", "Assets/_Scenes/0_Login.unity", "Assets/_Scenes/1_Lobby.unity", "Assets/_Scenes/2_Holdem.unity" };
        [MenuItem("MIMIC/Create or update game scenes")]
        public static void Generate()
        {
            if (!Application.isBatchMode && !EditorSceneManager.SaveCurrentModifiedScenesIfUserWantsTo()) return;
            font = AssetDatabase.LoadAssetAtPath<Font>("Assets/Art/Fonts/NotoSansCJKkr-Regular.otf");
            if (font == null) throw new System.Exception("Noto Korean font is missing.");
            Directory.CreateDirectory("Assets/_Scenes"); Directory.CreateDirectory("Assets/Prefabs/UI");
            Prefabs(); Title(); Login(); Lobby(); Holdem();
            var list = new EditorBuildSettingsScene[Scenes.Length];
            for (int i=0;i<list.Length;i++) list[i] = new EditorBuildSettingsScene(Scenes[i], true);
            EditorBuildSettings.scenes = list;
            PlayerSettings.companyName = "MIMIC"; PlayerSettings.productName = "MIMIC";
            PlayerSettings.SetApplicationIdentifier(UnityEditor.Build.NamedBuildTarget.Standalone, "com.mimic.holdem");
            PlayerSettings.defaultScreenWidth = 1600; PlayerSettings.defaultScreenHeight = 900;
            PlayerSettings.fullScreenMode = FullScreenMode.Windowed; PlayerSettings.runInBackground = true;
            PlayerSettings.insecureHttpOption = InsecureHttpOption.DevelopmentOnly;
            AssetDatabase.SaveAssets(); AssetDatabase.Refresh(); EditorSceneManager.OpenScene(Scenes[0]);
            Debug.Log("MIMIC_GAME_SCENES_READY: Title, Login, Lobby, Holdem; Canvas UI and prefabs saved.");
        }
        private static Color Hex(string hex) { ColorUtility.TryParseHtmlString("#"+hex, out var c); return c; }
        private static RectTransform Rect(Transform parent,string name,float x,float y,float w,float h)
        {
            var o=new GameObject(name,typeof(RectTransform));o.transform.SetParent(parent,false);
            var r=o.GetComponent<RectTransform>();r.anchorMin=r.anchorMax=new Vector2(0,1);r.pivot=new Vector2(0,1);r.anchoredPosition=new Vector2(x,-y);r.sizeDelta=new Vector2(w,h);return r;
        }
        private static RoundedPanel Box(Transform p,string name,float x,float y,float w,float h,Color c,float radius=22)
        {
            var panel=Rect(p,name,x,y,w,h).gameObject.AddComponent<RoundedPanel>();panel.color=c;panel.raycastTarget=false;
            var so=new SerializedObject(panel);so.FindProperty("radius").floatValue=radius;so.ApplyModifiedPropertiesWithoutUndo();return panel;
        }
        private static Text Text(Transform p,string name,string value,float x,float y,float w,float h,int size,Color c,TextAnchor align=TextAnchor.MiddleLeft)
        {
            var t=Rect(p,name,x,y,w,h).gameObject.AddComponent<Text>();t.font=font;t.fontSize=size;t.color=c;t.text=value;t.alignment=align;
            t.horizontalOverflow=HorizontalWrapMode.Wrap;t.verticalOverflow=VerticalWrapMode.Truncate;t.raycastTarget=false;t.supportRichText=false;return t;
        }
        private static Button Button(Transform p,string name,string value,float x,float y,float w,float h,bool primary=false)
        {
            var box=Box(p,name,x,y,w,h,primary?Gold:Light,14);box.raycastTarget=true;
            var b=box.gameObject.AddComponent<Button>();b.targetGraphic=box;
            var colors=b.colors;colors.highlightedColor=new Color(1.1f,1.1f,1.1f);colors.pressedColor=new Color(.8f,.8f,.8f);colors.disabledColor=new Color(.5f,.5f,.5f,.65f);b.colors=colors;
            Text(box.transform,"Label",value,10,0,w-20,h,24,primary?Bg:White,TextAnchor.MiddleCenter);return b;
        }
        private static InputField Field(Transform p,string name,string hint,float x,float y,float w,bool password=false,int limit=24)
        {
            var box=Box(p,name,x,y,w,70,Hex("0B1B28"),12);box.raycastTarget=true;
            var f=box.gameObject.AddComponent<InputField>();f.targetGraphic=box;f.characterLimit=limit;
            f.textComponent=Text(box.transform,"Value","",22,0,w-44,70,25,White);f.placeholder=Text(box.transform,"Placeholder",hint,22,0,w-44,70,23,Muted);
            f.contentType=password?InputField.ContentType.Password:InputField.ContentType.Standard;f.selectionColor=new Color(.32f,.64f,.65f,.6f);f.caretColor=Gold;f.customCaretColor=true;return f;
        }
        private static Toggle Toggle(Transform p,string name,string text,float x,float y,float w)
        {
            var r=Rect(p,name,x,y,w,40);var t=r.gameObject.AddComponent<Toggle>();
            var box=Box(r,"Box",0,4,30,30,Light,6);box.raycastTarget=true;
            t.targetGraphic=box;t.graphic=Text(box.transform,"Check","✓",0,0,30,30,25,Gold,TextAnchor.MiddleCenter);
            Text(r,"Label",text,43,0,w-43,40,20,Muted);return t;
        }
        private static Transform Base(string name)
        {
            EditorSceneManager.NewScene(NewSceneSetup.EmptyScene,NewSceneMode.Single);
            var camera=new GameObject("Camera").AddComponent<Camera>();camera.clearFlags=CameraClearFlags.SolidColor;camera.backgroundColor=Bg;camera.orthographic=true;camera.tag="MainCamera";
            var canvas=new GameObject(name+" Canvas",typeof(RectTransform),typeof(Canvas),typeof(CanvasScaler),typeof(GraphicRaycaster));
            canvas.GetComponent<Canvas>().renderMode=RenderMode.ScreenSpaceOverlay;
            var scale=canvas.GetComponent<CanvasScaler>();scale.uiScaleMode=CanvasScaler.ScaleMode.ScaleWithScreenSize;scale.referenceResolution=new Vector2(1920,1080);scale.matchWidthOrHeight=.5f;
            new GameObject("EventSystem",typeof(EventSystem),typeof(StandaloneInputModule));var root=canvas.transform;
            Box(root,"Background",0,0,1920,1080,Bg,0);Box(root,"Top divider",80,127,1760,1,Light,0);
            Text(root,"Brand","M I M I C",88,40,350,65,39,Gold);
            Text(root,"Edition","TEXAS HOLD'EM  /  ORIGINAL SERIES",1360,48,470,40,17,Muted,TextAnchor.MiddleRight);
            Text(root,"Footer","MIMIC  ·  PLAY WITH INTENTION",90,1023,600,32,15,Muted);
            Text(root,"DemoNotice","연습 칩 전용 · 실제 금전 거래 없음",1350,1023,480,32,16,Muted,TextAnchor.MiddleRight);return root;
        }
        private static void Common(ScreenBase s,Transform root)
        {
            s.status=Text(root,"Status","",470,970,980,40,20,Muted,TextAnchor.MiddleCenter);
            var busy=Box(root,"Loading overlay",0,0,1920,1080,new Color(.02f,.05f,.08f,.85f),0);busy.raycastTarget=true;
            var card=Box(busy.transform,"Loading card",700,410,520,220,PanelBg,24);
            Text(card.transform,"Brand","M I M I C",30,25,460,65,38,Gold,TextAnchor.MiddleCenter);
            Text(card.transform,"Message","연결 중입니다. 잠시만 기다려 주세요.",30,100,460,55,22,White,TextAnchor.MiddleCenter);
            busy.gameObject.SetActive(false);s.busyOverlay=busy.gameObject;
        }
        private static void ArtCard(Transform p,float x,float y,float angle,string rank,string suit,Color ink)
        {
            var shadow=Box(p,"Card shadow",x+14,y+18,242,354,new Color(0,0,0,.3f),20);shadow.rectTransform.localRotation=Quaternion.Euler(0,0,angle);
            var card=Box(p,"Card "+rank+suit,x,y,242,354,White,20);card.rectTransform.localRotation=Quaternion.Euler(0,0,angle);
            Text(card.transform,"Rank",rank,24,12,76,64,47,ink);Text(card.transform,"Suit corner",suit,24,76,70,45,35,ink);
            Text(card.transform,"Suit",suit,35,116,172,146,104,ink,TextAnchor.MiddleCenter);
            Text(card.transform,"Mark","M I M I C",25,300,192,26,14,Muted,TextAnchor.MiddleCenter);
        }
        private static void Save(int i) { EditorSceneManager.SaveScene(EditorSceneManager.GetActiveScene(),Scenes[i]); }
        private static void Title()
        {
            var r=Base("Title");var s=r.gameObject.AddComponent<TitleScreen>();
            Box(r,"Hero accent",1110,215,660,660,Hex("0D2B35"),330);Box(r,"Hero inset",1140,245,600,600,Hex("12313B"),300);
            Text(r,"Kicker","THE NEXT HAND IS YOURS.",110,242,900,45,20,Gold);
            Text(r,"Hero title","MIMIC",100,303,1040,170,136,White);
            Text(r,"Headline","당신만의 한 수를 보여주세요.",112,507,1040,80,44,White);
            Text(r,"Description","차분하게 읽고, 대담하게 플레이하세요.\n새로운 홀덤 테이블이 당신을 기다립니다.",115,611,900,95,26,Muted);
            s.startButton=Button(r,"Start game","게임 시작   →",115,753,368,83,true);s.guideButton=Button(r,"Guide","플레이 가이드",506,753,280,83);s.quitButton=Button(r,"Quit","종료",810,753,135,83);
            ArtCard(r,1230,340,14,"K","♠",Bg);ArtCard(r,1425,288,-12,"A","♦",Hex("B83F48"));
            Box(r,"Hero pill",1200,793,525,63,Bg,31);Text(r,"Hero pill label","NO LIMIT  ·  2–6 PLAYERS",1215,802,495,40,19,Gold,TextAnchor.MiddleCenter);
            Text(r,"Footnote","한 판의 시작, 당신의 선택.",116,902,900,45,20,Muted);
            var guide=Box(r,"Guide modal",0,0,1920,1080,new Color(.02f,.05f,.08f,.94f),0);guide.raycastTarget=true;
            var body=Box(guide.transform,"Guide card",470,235,980,585,PanelBg,24);
            Text(body.transform,"Title","첫 핸드를 위한 가이드",60,50,860,75,38,White);
            Text(body.transform,"Steps","01   계정을 만들거나 로그인하세요.\n02   로비에서 홀덤 테이블에 입장하세요.\n03   준비를 누르면 2명부터 게임이 시작됩니다.\n04   폴드 · 체크 · 콜 · 레이즈로 플레이하세요.",60,160,850,270,26,Muted);
            s.closeGuideButton=Button(body.transform,"Close","확인",60,460,860,70,true);guide.gameObject.SetActive(false);s.guide=guide.gameObject;
            Common(s,r);Save(0);
        }
        private static void Login()
        {
            var r=Base("Login");var s=r.gameObject.AddComponent<LoginScreen>();
            var hero=Box(r,"Login story",90,175,800,770,Hex("0D2B35"),28);
            Text(hero.transform,"Kicker","WELCOME TO THE TABLE",60,48,660,50,20,Gold);
            Text(hero.transform,"Heading","좋은 플레이는\n당신으로부터.",58,131,650,160,58,White);
            Text(hero.transform,"Subheading","당신의 이름으로 시작하는 한 판.\nMIMIC에서 나만의 플레이를 만들어보세요.",60,330,660,100,25,Muted);
            ArtCard(hero.transform,358,470,10,"A","♠",Bg);ArtCard(hero.transform,540,440,-9,"A","♥",Hex("B83F48"));
            s.back=Button(hero.transform,"Back","←  시작 화면",60,665,235,60);
            var form=Rect(r,"Account form",1030,140,720,900);
            s.heading=Text(form,"Heading","다시 만나 반가워요",0,0,700,70,40,White);
            s.subheading=Text(form,"Subheading","계정으로 로그인하고 테이블에 합류하세요.",0,75,700,44,21,Muted);
            s.loginTab=Button(form,"Login tab","로그인",0,142,318,57);s.registerTab=Button(form,"Register tab","회원가입",342,142,318,57);
            Text(form,"Account label","아이디",0,216,600,32,21,White);s.account=Field(form,"Account","영문·숫자·밑줄 3~24자",0,255,660);
            Text(form,"Password label","비밀번호",0,339,600,32,21,White);s.password=Field(form,"Password","8자 이상 입력해 주세요",0,378,660,true,128);
            var extra=Rect(form,"Registration fields",0,463,660,230);
            Text(extra,"Nickname label","닉네임",0,0,600,30,21,White);s.nickname=Field(extra,"Nickname","테이블에서 사용할 이름 2~24자",0,32,660);
            Text(extra,"Confirm label","비밀번호 확인",0,109,600,30,21,White);s.confirmPassword=Field(extra,"Confirm password","비밀번호를 한 번 더 입력해 주세요",0,143,660,true,128);
            s.registerFields=extra.gameObject;extra.gameObject.SetActive(false);
            s.optionsRow=Rect(form,"Options",0,470,660,42);s.rememberAccount=Toggle(s.optionsRow,"Remember account","아이디 기억하기",0,0,320);s.showPassword=Toggle(s.optionsRow,"Show password","비밀번호 표시",365,0,295);
            s.validation=Text(form,"Validation","",0,520,660,48,19,Hex("FF8075"));s.validationRect=s.validation.rectTransform;
            s.submit=Button(form,"Submit","로그인",0,580,660,82,true);s.submitRow=s.submit.GetComponent<RectTransform>();s.submitLabel=s.submit.GetComponentInChildren<Text>();
            Common(s,r);Object.DestroyImmediate(s.status.gameObject);s.status=s.validation;Save(1);
        }
        private static void Prefabs()
        {
            var temp=new GameObject("Prefab workspace");var card=Box(temp.transform,"Holdem table card",0,0,940,365,PanelBg,24);var v=card.gameObject.AddComponent<TableCardView>();
            Text(card.transform,"Tag","TEXAS HOLD'EM",34,22,520,38,16,Gold);v.title=Text(card.transform,"Title","홀덤 테이블 01",32,63,600,64,37,White);
            v.description=Text(card.transform,"Description","NO LIMIT HOLD'EM  ·  연습 칩",35,136,620,36,17,Muted);
            Text(card.transform,"Blinds label","블라인드",35,204,140,34,18,Muted);v.blinds=Text(card.transform,"Blinds","10 / 20",35,246,330,55,35,White);
            v.seats=Text(card.transform,"Players","0 / 6 플레이어",420,264,260,46,21,Muted);
            Box(card.transform,"Table art outer",710,42,180,104,Teal,52);Box(card.transform,"Table art felt",724,56,152,76,Hex("103039"),38);Text(card.transform,"Table emblem","M",742,67,116,53,34,Gold,TextAnchor.MiddleCenter);
            v.join=Button(card.transform,"Join","입장하기  →",694,261,213,66,true);
            tablePrefab=PrefabUtility.SaveAsPrefabAsset(card.gameObject,"Assets/Prefabs/UI/HoldemTableCard.prefab").GetComponent<TableCardView>();Object.DestroyImmediate(card.gameObject);
            var seat=Box(temp.transform,"Holdem seat",0,0,250,148,Light,20);var sv=seat.gameObject.AddComponent<HoldemSeatView>();sv.outline=seat;Box(seat.transform,"Body",2,2,246,144,PanelBg,19);
            sv.playerName=Text(seat.transform,"Name","빈 자리",14,9,222,32,21,White,TextAnchor.MiddleCenter);sv.chips=Text(seat.transform,"Chips","",14,45,222,33,25,Gold,TextAnchor.MiddleCenter);
            sv.cards=Text(seat.transform,"Cards","",14,78,222,36,28,White,TextAnchor.MiddleCenter);sv.state=Text(seat.transform,"State","",14,116,222,24,15,Muted,TextAnchor.MiddleCenter);
            seatPrefab=PrefabUtility.SaveAsPrefabAsset(seat.gameObject,"Assets/Prefabs/UI/HoldemSeat.prefab").GetComponent<HoldemSeatView>();Object.DestroyImmediate(temp);
        }
        private static void Lobby()
        {
            var r=Base("Lobby");var s=r.gameObject.AddComponent<LobbyScreen>();
            Text(r,"Kicker","YOUR SEAT IS WAITING",92,179,1100,40,18,Gold);Text(r,"Heading","어떤 한 판을 시작할까요?",87,231,1200,93,53,White);s.greeting=Text(r,"Greeting","로비에 오신 것을 환영합니다.",91,331,1180,43,24,Muted);
            var nav=Box(r,"Games navigation",90,419,272,518,PanelBg,22);Text(nav.transform,"Games heading","게임",28,27,210,40,22,Muted);Box(nav.transform,"Holdem selected",17,99,238,70,Teal,12);Text(nav.transform,"Holdem label","♠   텍사스 홀덤",35,110,207,46,23,White);
            Text(nav.transform,"Details","NO LIMIT\n\n2~6인 테이블\n연습 칩으로 플레이",29,238,220,203,20,Muted);
            s.tableCount=Text(r,"Table count","플레이 가능한 테이블  1",407,414,650,50,23,White);s.refresh=Button(r,"Refresh","새로고침",1158,408,188,54);
            var list=Rect(r,"Table list",405,488,950,378);var grid=list.gameObject.AddComponent<GridLayoutGroup>();grid.cellSize=new Vector2(940,365);grid.spacing=new Vector2(0,24);grid.constraint=GridLayoutGroup.Constraint.FixedColumnCount;grid.constraintCount=1;s.tablesRoot=list;s.tablePrefab=tablePrefab;
            s.emptyState=Text(r,"Empty tables","테이블 목록을 불러오는 중입니다.",426,610,860,70,26,Muted,TextAnchor.MiddleCenter).gameObject;
            var note=Box(r,"Lobby note",405,879,940,58,Light,12);Text(note.transform,"Note","입장 후 준비를 눌러주세요. 2명 이상이 준비하면 게임이 시작됩니다.",18,8,904,42,18,Muted);
            var profile=Box(r,"Player profile",1430,179,400,758,PanelBg,24);Box(profile.transform,"Avatar",36,32,82,82,Teal,41);Text(profile.transform,"Avatar letter","M",37,34,80,78,39,Gold,TextAnchor.MiddleCenter);
            Text(profile.transform,"Profile label","MY ACCOUNT",144,33,235,33,16,Gold);s.accountName=Text(profile.transform,"Account name","",144,74,225,40,23,White);s.connection=Text(profile.transform,"Connection","● 연결됨",37,159,323,37,18,Hex("73C7A9"));
            Box(profile.transform,"Divider",35,218,330,1,Light,0);Text(profile.transform,"Balance label","연습 칩",36,251,320,45,23,Muted);s.balance=Text(profile.transform,"Balance","1,000",31,311,337,87,58,Gold);
            Text(profile.transform,"Practice note","가볍게 시작하고,\n당신의 플레이를 완성하세요.",36,440,330,92,21,Muted);s.quickStart=Button(profile.transform,"Quick start","빠른 입장   →",35,576,330,75,true);s.logout=Button(profile.transform,"Logout","로그아웃",35,673,330,51);
            Common(s,r);Save(2);
        }
        private static void Holdem()
        {
            var r=Base("Holdem");var s=r.gameObject.AddComponent<HoldemScreen>();Text(r,"Room name","TABLE 01   /   NO LIMIT HOLD'EM   /   10 · 20",450,51,1050,47,23,White);s.leave=Button(r,"Leave table","로비로",1590,40,230,61);
            Box(r,"Table edge",333,258,1254,532,Hex("244B50"),265);Box(r,"Table rail",345,270,1230,508,Hex("152D37"),254);Box(r,"Felt",375,300,1170,448,Hex("15434A"),224);
            Text(r,"Felt brand","M I M I C",646,649,630,48,29,new Color(.32f,.53f,.52f),TextAnchor.MiddleCenter);s.stage=Text(r,"Street","WAITING FOR PLAYERS",650,302,620,41,17,Gold,TextAnchor.MiddleCenter);
            Text(r,"Pot label","TOTAL POT",790,355,340,35,17,Muted,TextAnchor.MiddleCenter);s.pot=Text(r,"Pot","0",790,393,340,57,40,White,TextAnchor.MiddleCenter);
            s.board=new Text[5];for(int i=0;i<5;i++){var c=Box(r,"Board card "+(i+1),680+i*115,474,99,136,White,12);s.board[i]=Text(c.transform,"Card face","M",5,5,89,126,32,Muted,TextAnchor.MiddleCenter);}
            Vector2[] places={new Vector2(835,767),new Vector2(299,661),new Vector2(299,205),new Vector2(835,149),new Vector2(1370,205),new Vector2(1370,661)};
            s.seats=new HoldemSeatView[6];for(int i=0;i<6;i++){var seat=(GameObject)PrefabUtility.InstantiatePrefab(seatPrefab.gameObject,r);seat.name="Seat "+(i+1);seat.GetComponent<RectTransform>().anchoredPosition=new Vector2(places[i].x,-places[i].y);s.seats[i]=seat.GetComponent<HoldemSeatView>();}
            s.instruction=Text(r,"Turn instruction","모든 플레이어가 준비하면 게임이 시작됩니다.",510,706,900,41,23,White,TextAnchor.MiddleCenter);s.result=Text(r,"Hand result","",480,850,960,44,23,Gold,TextAnchor.MiddleCenter);
            s.ready=Button(r,"Ready","준비하기",755,922,410,76,true);var bar=Rect(r,"Action bar",491,922,1120,80);s.actionBar=bar.gameObject;
            s.fold=Button(bar,"Fold","폴드",0,0,178,76);s.call=Button(bar,"Check or call","체크",196,0,210,76);s.callLabel=s.call.GetComponentInChildren<Text>();s.raiseAmount=Field(bar,"Raise to","총 베팅금액",426,3,196,false,10);s.raiseAmount.contentType=InputField.ContentType.IntegerNumber;s.raise=Button(bar,"Raise","레이즈",641,0,228,76,true);bar.gameObject.SetActive(false);
            Common(s,r);s.status.rectTransform.anchoredPosition=new Vector2(450,-1016);s.status.rectTransform.sizeDelta=new Vector2(1020,40);Save(3);
        }
    }
}
