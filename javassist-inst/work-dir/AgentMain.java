public class AgentMain {

    public static void agentmain (String agentArgs, Instrumentation inst) throws Exception {
        TransformerService ts = new TransformerService(inst);
        ObjectName on = new ObjectName("transformer:service=DemoTransformer");
        // Could be a different MBeanServer. If so, pass a JMX Default Domain Name in agentArgs
        MBeanServer server = ManagementFactory.getPlatformMBeanServer();
        server.registerMBean(ts, on);
        // Set this property so the installer knows we're already here
        System.setProperty("demo.agent.installed", "true");     
    }

}