using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace CryLinkHelper
{
	class CryLinkService
	{
		/// <summary>
		/// Gets the connection string.
		/// </summary>
		public String Connection { get; private set; }

		/// <summary>
		/// Creates a new instance with a connection string.
		/// </summary>
		/// <param name="host">Host name or ip.</param>
		/// <param name="port">Port to use.</param>
		public CryLinkService(String host, Int32 port)
		{
			Connection = "http://" + host + ":" + port + "/rpc2";
		}

		/// <summary>
		/// Requests a challenge from the service.
		/// </summary>
		/// <returns>Challenge value or 0 if failed.</returns>
		public Int64 Challenge()
		{
			try
			{
				XmlRPCMethod method = new XmlRPCMethod(Connection, "challenge");
				method.KeepAlive = true;
				
				return method.Call<Int64>(null);
			}
			catch (Exception)
			{
				return 0;
			}
		}

		/// <summary>
		/// Sends an authentication to the service.
		/// </summary>
		/// <param name="auth">Auth string</param>
		/// <returns>True if authentication succeeded.</returns>
		public bool Authenticate(String auth)
		{
			try
			{
				XmlRPCMethod method = new XmlRPCMethod(Connection, "authenticate");
				method.KeepAlive = true;
				
				return method.Call<bool>(new ArrayList() { auth });
			}
			catch (Exception)
			{
				return false;
			}
		}

		/// <summary>
		/// Executes a console command.
		/// </summary>
		/// <param name="command">Console command to send</param>
		/// <returns>Command return value as string.</returns>
		public String ExecuteCommand(CryConsoleCommand command)
		{
			try
			{
				XmlRPCMethod method = new XmlRPCMethod(Connection, command.Name);
				method.KeepAlive = true;

				return method.Call<String>(new ArrayList(command.Params));
			}
			catch (Exception)
			{
				return "";
			}
		}

		/// <summary>
		/// Generates CryLinkService auth string.
		/// </summary>
		/// <param name="challenge">Challenge that was sent by the service.</param>
		/// <param name="password">Service password.</param>
		/// <returns></returns>
		public static String GenerateAuth(Int64 challenge, String password)
		{
			byte[] md5Data;
			using (System.Security.Cryptography.MD5 md5Hash = System.Security.Cryptography.MD5.Create())
			{
				String compose = challenge + ":" + password;
				md5Data = md5Hash.ComputeHash(Encoding.UTF8.GetBytes(compose));
			}

			StringBuilder stringBuilder = new StringBuilder();
			for (Int32 i = 0; i < md5Data.Length; ++i)
			{
				stringBuilder.Append(md5Data[i].ToString("x2"));
			}

			return stringBuilder.ToString();
		}
	}
}
