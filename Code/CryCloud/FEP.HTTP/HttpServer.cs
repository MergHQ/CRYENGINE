using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Net;
using log4net;
using System.Threading;
using System.IO;
using System.Web;
using CryBackend;
using AustinHarris.JsonRpc;
using Nancy;
using Nancy.Authentication.Basic;
using Nancy.Bootstrapper;
using Nancy.TinyIoc;
using Nancy.Hosting.Self;
using Nancy.Session;

namespace CryBackend.FEP.Http
{
    public class HttpServer : IDisposable
    {
        private readonly List<NancyHost> m_hosts;
        private readonly ILog m_logger;
        private string[] m_urls;
    
        public HttpServer(string[] urls)
        {
            m_logger = LogManager.GetLogger("HttpServer");
            m_hosts = new List<NancyHost> {};
            this.m_urls = urls;
            HostConfiguration hostConf = new HostConfiguration();
            hostConf.RewriteLocalhost = true;
            hostConf.UrlReservations.CreateAutomatically = true;
            foreach (var url in urls)
            {
                var newurl = url.Replace("0.0.0.0", "localhost");
                m_hosts.Add(new NancyHost(hostConf,  new Uri(newurl)));
            }
        }

        public void Start()
        {
            m_logger.InfoFormat("Starting Http server on {0}", m_urls.Aggregate("", (old, e) => old + e + ";"));
            foreach (var host in m_hosts)
            {
                host.Start();
            }
        }

        public void Stop()
        {
            foreach (var host in m_hosts)
            {
                host.Stop();
            }
        }

        public void Dispose()
        {
            Stop();
        }
    }

    public class Bootstrapper : DefaultNancyBootstrapper
    {
        protected override void ApplicationStartup(TinyIoCContainer container, IPipelines pipelines)
        {
            CookieBasedSessions.Enable(pipelines);
            /*
            pipelines.EnableBasicAuthentication(new BasicAuthenticationConfiguration(
                container.Resolve<IUserValidator>(),
                "MyRealm"));
                */
            base.ApplicationStartup(container, pipelines);
        }
    }

}
