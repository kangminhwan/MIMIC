using Google.Protobuf;
using Mimic.Protocol;

var builder = WebApplication.CreateBuilder(args);
builder.WebHost.UseUrls(builder.Configuration["MIMIC_FRONT_URL"] ?? "http://127.0.0.1:5080");
var app = builder.Build();
app.MapGet("/health", () => Results.Ok(new { service = "MIMIC.FrontServer", status = "ok" }));
app.MapGet("/portal", () => Results.Bytes(new PortalReply
{
    Product = "MIMIC",
    PlatformUrl = app.Configuration["MIMIC_PLATFORM_PUBLIC_URL"] ?? "http://127.0.0.1:5081",
    TableHost = app.Configuration["MIMIC_TABLE_PUBLIC_HOST"] ?? "127.0.0.1",
    TablePort = uint.Parse(app.Configuration["MIMIC_TABLE_PORT"] ?? "7777"),
    ProtocolVersion = 1
}.ToByteArray(), "application/x-protobuf"));
app.Run();
