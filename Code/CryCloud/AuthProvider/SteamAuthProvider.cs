using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Text;
using System.Threading.Tasks;
using System.Xml.Linq;
using FP;
using log4net;
using Orleans;
using Newtonsoft.Json;

namespace CryBackend.AuthProvider
{


    /*
    This is a success response
       {
         "response": {
           "params": {
             "result": "OK",
             "steamid": "76561198392705522",
             "ownersteamid": "76561198392705522",
             "vacbanned": false,
             "publisherbanned": false
           }
        }

    This is an error with invalid parameter
        {
         "response": {
            "error": {
              "errorcode": 3,
              "errordesc": "Invalid parameter"
            }
          }
        }

    This is an error with logical error

        {
          "response": {
            "params": {
              "result": "Invalid ticket",
              "steamid": "76561198392705522",
              "ownersteamid": "0",
              "vacbanned": false,
              "publisherbanned": false
            }
          }
        }
     */
    class SteamResponse
    {
        [JsonProperty("response")]
        public SteamParams Response { get; set; }
    }

    class SteamParams
    {
        [JsonProperty("params")]
        public SteamAuthenticateUserTicketResponse Params { get; set; }
        [JsonProperty("error")]
        public SteamError Error { get; set; }
    }

    class SteamError
    {
        [JsonProperty("errorcode")]
        public int ErrorCode { get; set; }

        [JsonProperty("errordesc")]
        public string ErrorDesc { get; set; }
    }

    class SteamAuthenticateUserTicketResponse
    {
        [JsonProperty("result")]
        public string Result { get; set; }
        [JsonProperty("steamid")]
        public string SteamId { get; set; }
        [JsonProperty("ownersteamid")]
        public string OwnerSteamId { get; set; }
        [JsonProperty("vacbanned")]
        public bool VacBanned { get; set; }
        [JsonProperty("publisherbanned")]
        public bool PublisherBanned { get; set; }
    }

    [AuthProvider]
    public class SteamAuthProvider : IAuthProvider
    {
	    private static ILog Logger { get; } = LogManager.GetLogger(typeof(SteamAuthProvider).Name);

        private readonly string m_apiKey = "";
        private readonly string m_appId = "";
        private readonly string m_steamUrl = "https://api.steampowered.com";
        public SteamAuthProvider(XElement config)
        {
            m_apiKey = ConfigUtils.ReadStringValue(config, "ApiKey", ""); 
            m_appId = ConfigUtils.ReadStringValue(config, "AppId", ""); 
        }

        public Task<AuthResult> AuthLogin(AuthLoginReq req, string[] permissions)
        {
	        if (req.SessionType == ESessionType.Client)
	        {
#if RELEASE
				if (permissions != null && permissions.Any(e => e != "everyone"))
		        {
			        Logger.Warn("A user breached to an internal gateway and trying to login! Contact NetOps!!");
					return Task.FromResult(new AuthResult { Authorized = false });
				}
#endif //RELEASE

				if (req.CredentialCase == AuthLoginReq.CredentialOneofCase.Token)
			        return DoAuthCheckToken(req.Token);
	        }

			return Task.FromResult(new AuthResult { Authorized = false });
		}

		async Task<AuthResult> DoAuthCheckToken(string token)
        {
            string addresss = m_steamUrl + "/ISteamUserAuth/AuthenticateUserTicket/v0001?key={0}&appid={1}&ticket={2}";

            addresss = String.Format(addresss, m_apiKey, m_appId, token);

            WebRequest request = WebRequest.Create(addresss);
            request.Method = "GET";

            try
            {
                WebResponse webResponse = await Task.Factory.FromAsync<WebResponse>(request.BeginGetResponse,
                    request.EndGetResponse,
                    null);
        
                using (HttpWebResponse response = (HttpWebResponse)webResponse)
                {
                    using (Stream dataStream = response.GetResponseStream())
                    {
                        using (StreamReader reader = new StreamReader(dataStream))
                        {
                            string returnStr = await reader.ReadToEndAsync();
                            var data = JsonConvert.DeserializeObject<SteamResponse>(returnStr);
                            if (data.Response.Error != null)
                            {
                                return new FP.AuthResult { Authorized = false };

                            }
                            if (data.Response.Params.Result != "OK" || data.Response.Params.VacBanned || data.Response.Params.PublisherBanned)
                            {
                                return new FP.AuthResult { Authorized = false };
                            }
                            var result = new FP.AuthResult { Authorized = true, Token = token, UserId = Convert.ToInt64(data.Response.Params.SteamId)};
                            return result;
                        }
                    }
                }
            }
            catch
            {
                return new FP.AuthResult { Authorized = false };
            }
        }

        public Task AuthLogout(string authReqToken)
        {
            return TaskDone.Done;
        }

        public void Dispose()
        {
        }
    }
}
