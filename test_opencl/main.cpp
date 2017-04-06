#include "Poco/File.h"
#include <iostream>
#include "stdafx.h"
#include "HelloWorldCL.h"

Util::LayeredConfiguration *g_cfg = NULL;
Logger *g_log = NULL;

class StitcherAPP : public Application
{
public:
	StitcherAPP()
	{
	}

	~StitcherAPP()
	{
	}

protected:
	void initialize(Application& self)
	{
		loadConfiguration("./config.xml"); // load default configuration files, if present
		Application::initialize(self);

		File logDir(Path(config().getString("application.dir")).append("log"));
		if (!logDir.exists())
			logDir.createDirectory();
	}

	void uninitialize()
	{
		Application::uninitialize();
	}

	void defineOptions(OptionSet& options)
	{
		Application::defineOptions(options);
	}

	int main(const ArgVec& args)
	{
		g_cfg = &config();
		g_log = &logger();
		TRY;
		
		HelloWorldCL hcl;
		hcl.Init("./test.cl");

		CATCH;
		return Application::EXIT_OK;
	}
};

POCO_APP_MAIN(StitcherAPP)

