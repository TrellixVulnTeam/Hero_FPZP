
#include "baseapp.h"
#include "baseapp_interface.h"
#include "initprogress_handler.h"
#include "network/common.h"
#include "network/tcp_packet.h"
#include "network/udp_packet.h"
#include "network/message_handler.h"
#include "network/bundle_broadcast.h"
#include "thread/threadpool.h"
#include "server/components.h"
#include <sstream>

#include "proto/cb.pb.h"
//#include "proto/up.pb.h"
#include "proto/bmb.pb.h"
#include "../../server/basemgr/basemgr_interface.h"
#include "proto/basedb.pb.h"
#include "../../server/dbmgr/dbmgr_interface.h"

namespace KBEngine{
	
ServerConfig g_serverConfig;
KBE_SINGLETON_INIT(BaseApp);

//-------------------------------------------------------------------------------------
BaseApp::BaseApp(Network::EventDispatcher& dispatcher, 
				 Network::NetworkInterface& ninterface, 
				 COMPONENT_TYPE componentType,
				 COMPONENT_ID componentID):
	EntityApp<Base>(dispatcher, ninterface, componentType, componentID),
	loopCheckTimerHandle_(),
	pendingLoginMgr_(),
	idClient_(),
	pInitProgressHandler_(NULL),
	numProxices_(0)
{
	idClient_.pApp(this);
}

//-------------------------------------------------------------------------------------
BaseApp::~BaseApp()
{
	// 不需要主动释放
	pInitProgressHandler_ = NULL;
}

//-------------------------------------------------------------------------------------		
bool BaseApp::initializeWatcher()
{
	return true;
}

//-------------------------------------------------------------------------------------
bool BaseApp::run()
{
	dispatcher_.processUntilBreak();
	return true;
}

//-------------------------------------------------------------------------------------
void BaseApp::handleTimeout(TimerHandle handle, void * arg)
{
	switch (reinterpret_cast<uintptr>(arg))
	{
		case TIMEOUT_TICK:
			this->handleTick();
			break;
		default:
			break;
	}

	ServerApp::handleTimeout(handle, arg);
}

//-------------------------------------------------------------------------------------
void BaseApp::handleTick()
{

	threadPool_.onMainThreadTick();
	networkInterface().processChannels(&BaseappInterface::messageHandlers);
}

//-------------------------------------------------------------------------------------
bool BaseApp::initializeBegin()
{
	return true;
}

////-------------------------------------------------------------------------------------
//bool BaseApp::inInitialize()
//{
//	return true;
//}

//-------------------------------------------------------------------------------------
bool BaseApp::initializeEnd()
{
	loopCheckTimerHandle_ = this->dispatcher().addTimer(1000000 / 50, this,
							reinterpret_cast<void *>(TIMEOUT_TICK));

	//new GameModule();
	//if (!GameModule::getSingleton().InitModule())
	//{
	//	return false;
	//}
	return true;
}

//-------------------------------------------------------------------------------------
void BaseApp::finalise()
{

	loopCheckTimerHandle_.cancel();
	ServerApp::finalise();
}

//-------------------------------------------------------------------------------------	
bool BaseApp::canShutdown()
{
	if(Components::getSingleton().getGameSrvComponentsSize() > 0)
	{
		INFO_MSG(fmt::format("BaseApp::canShutdown(): Waiting for components({}) destruction!\n", 
			Components::getSingleton().getGameSrvComponentsSize()));

		return false;
	}

	return true;
}

//-------------------------------------------------------------------------------------
void BaseApp::onChannelDeregister(Network::Channel * pChannel)
{
	ENTITY_ID pid = pChannel->proxyID();

	// 如果是cellapp死亡了
	if (pChannel->isInternal())
	{
		Components::ComponentInfos* cinfo = Components::getSingleton().findComponent(pChannel);
		if (cinfo)
		{
			if (cinfo->componentType == CELLAPP_TYPE)
			{
				//onCellAppDeath(pChannel);
			}
		}
	}

	EntityApp<Base>::onChannelDeregister(pChannel);

	// 有关联entity的客户端退出则需要设置entity的client
	if (pid > 0)
	{
		Proxy* proxy = static_cast<Proxy*>(this->findEntity(pid));
		if (proxy)
		{
			proxy->onClientDeath();
		}
	}
}

//-------------------------------------------------------------------------------------
Base* BaseApp::onCreateEntity(PyObject* pyEntity, ENTITY_ID eid)
{
	if (PyType_IsSubtype(pyEntity->ob_type, Proxy::getScriptType()))
	{
		Proxy* pproxy = new(pyEntity)Proxy(eid);
		return pproxy;
	}

	return EntityApp<Base>::onCreateEntity(pyEntity, eid);
}

//-------------------------------------------------------------------------------------
bool BaseApp::installPyModules()
{
	Base::installScript(getScript().getModule());
	Proxy::installScript(getScript().getModule());

	registerScript(Base::getScriptType());
	registerScript(Proxy::getScriptType());

	// 注册创建entity的方法到py 
	//APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(), time, __py_gametime, METH_VARARGS, 0);
	//APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(), createBase, __py_createBase, METH_VARARGS, 0);
	//APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(), createBaseLocally, __py_createBase, METH_VARARGS, 0);
	//APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(), createEntity, __py_createBase, METH_VARARGS, 0);
	//APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(), createBaseAnywhere, __py_createBaseAnywhere, METH_VARARGS, 0);
	//APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(), createBaseFromDBID, __py_createBaseFromDBID, METH_VARARGS, 0);
	//APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(), createBaseAnywhereFromDBID, __py_createBaseAnywhereFromDBID, METH_VARARGS, 0);
	//APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(), executeRawDatabaseCommand, __py_executeRawDatabaseCommand, METH_VARARGS, 0);
	//APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(), quantumPassedPercent, __py_quantumPassedPercent, METH_VARARGS, 0);
	//APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(), charge, __py_charge, METH_VARARGS, 0);
	//APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(), registerReadFileDescriptor, PyFileDescriptor::__py_registerReadFileDescriptor, METH_VARARGS, 0);
	//APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(), registerWriteFileDescriptor, PyFileDescriptor::__py_registerWriteFileDescriptor, METH_VARARGS, 0);
	//APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(), deregisterReadFileDescriptor, PyFileDescriptor::__py_deregisterReadFileDescriptor, METH_VARARGS, 0);
	//APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(), deregisterWriteFileDescriptor, PyFileDescriptor::__py_deregisterWriteFileDescriptor, METH_VARARGS, 0);
	//APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(), reloadScript, __py_reloadScript, METH_VARARGS, 0);
	//APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(), isShuttingDown, __py_isShuttingDown, METH_VARARGS, 0);
	//APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(), address, __py_address, METH_VARARGS, 0);
	//APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(), deleteBaseByDBID, __py_deleteBaseByDBID, METH_VARARGS, 0);
	//APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(), lookUpBaseByDBID, __py_lookUpBaseByDBID, METH_VARARGS, 0);
	//APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(), setAppFlags, __py_setFlags, METH_VARARGS, 0);
	//APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(), getAppFlags, __py_getFlags, METH_VARARGS, 0);

	return EntityApp<Base>::installPyModules();
}

//-------------------------------------------------------------------------------------
void BaseApp::onInstallPyModules()
{
	// 添加globalData, globalBases支持

	if (PyModule_AddIntMacro(this->getScript().getModule(), LOG_ON_REJECT))
	{
		ERROR_MSG("Baseapp::onInstallPyModules: Unable to set KBEngine.LOG_ON_REJECT.\n");
	}

	if (PyModule_AddIntMacro(this->getScript().getModule(), LOG_ON_ACCEPT))
	{
		ERROR_MSG("Baseapp::onInstallPyModules: Unable to set KBEngine.LOG_ON_ACCEPT.\n");
	}

	if (PyModule_AddIntMacro(this->getScript().getModule(), LOG_ON_WAIT_FOR_DESTROY))
	{
		ERROR_MSG("Baseapp::onInstallPyModules: Unable to set KBEngine.LOG_ON_WAIT_FOR_DESTROY.\n");
	}
}

//-------------------------------------------------------------------------------------
bool BaseApp::uninstallPyModules()
{
	//if (g_kbeSrvConfig.getBaseApp().profiles.open_pyprofile)
	//{
	//	script::PyProfile::stop("kbengine");

	//	char buf[MAX_BUF];
	//	kbe_snprintf(buf, MAX_BUF, "baseapp%u.pyprofile", startGroupOrder_);
	//	script::PyProfile::dump("kbengine", buf);
	//	script::PyProfile::remove("kbengine");
	//}

	//unregisterPyObjectToScript("baseAppData");
	//S_RELEASE(pBaseAppData_);

	Base::uninstallScript();
	Proxy::uninstallScript();
	return EntityApp<Base>::uninstallPyModules();
}

bool BaseApp::installEntityDef()
{
	if (!loadUnitScriptType("Player"))
	{
		return false;
	}
	//Proxy* base = static_cast<Proxy*>(createEntity("Player", 5));
	//PyObject* pObj2 = dynamic_cast<PyObject*>(base);
	//
	//PyObject* pObj = static_cast<PyObject*>(base);
	//PyObject* pObj3 = (PyObject*)(base);
	//PyObject* pObj5 = (PyObject*)((DWORD)base);
	//printf("base:%d, pObj:%d, %d, %d \n", base, pObj, pObj3, pObj5);
	//SCRIPT_OBJECT_CALL_ARGS0(pObj5, const_cast<char*>("onUserLogonOn"), true);
	//SCRIPT_OBJECT_CALL_ARGS0(pObj, const_cast<char*>("onUserLogonOn"), true);
	//SCRIPT_OBJECT_CALL_ARGS0(pObj2, const_cast<char*>("onUserLogonOn"), true);
	//SCRIPT_OBJECT_CALL_ARGS0(pObj3, const_cast<char*>("onUserLogonOn"), true);
	//base->onUserLogonOn();
	return true;
}

////-------------------------------------------------------------------------------------
//void BaseApp::handleCheckStatusTick()
//{
//	pendingLoginMgr_.process();
//}

void BaseApp::registerPendingLogin(Network::Channel* pChannel, MemoryStream& s)
{
	if (pChannel->isExternal())
	{
		s.done();
		return;
	}

	std::string									loginName;
	std::string									accountName;
	std::string									password;
	std::string									datas;
	ENTITY_ID									entityID;
	DBID										entityDBID;
	uint32										flags;

	basemgr_base::registerPendingLogin rplCmd;
	PARSEBUNDLE(s, rplCmd)
	loginName = rplCmd.loginname();
	accountName = rplCmd.accountname();
	password = rplCmd.password();
	datas = rplCmd.extradata();
	entityID = rplCmd.eid();
	entityDBID = rplCmd.dbid();
	flags = rplCmd.flags();

	Network::Bundle* pBundle = Network::Bundle::ObjPool().createObject();
	(*pBundle).newMessage(BaseappmgrInterface::onPendingAccountGetBaseappAddr);

	basemgr_base::PendingAccountGetBaseappAddr pagbaCmd;
	pagbaCmd.set_loginname(loginName);
	pagbaCmd.set_accountname(accountName);
	pagbaCmd.set_ip(inet_ntoa((struct in_addr&)networkInterface().extaddr().ip));
	pagbaCmd.set_port(this->networkInterface().extaddr().port);

	ADDTOBUNDLE((*pBundle), pagbaCmd)
	pChannel->send(pBundle);

	PendingLoginMgr::PLInfos* ptinfos = new PendingLoginMgr::PLInfos;
	ptinfos->accountName = accountName;
	ptinfos->password = password;
	ptinfos->entityID = entityID;
	ptinfos->entityDBID = entityDBID;
	ptinfos->flags = flags;
	ptinfos->datas = datas;
	pendingLoginMgr_.add(ptinfos);
}


void BaseApp::onDbmgrInitCompleted(Network::Channel* pChannel, MemoryStream& s)
{
	if (pChannel->isExternal())
		return;
	DEBUG_MSG(fmt::format("BaseApp::onDbmgrInitCompleted\n"));
	base_dbmgr::DbmgrInitCompleted dicCmd;
	PARSEBUNDLE(s, dicCmd);

	idClient_.onAddRange(dicCmd.startentityid(), dicCmd.endentityid());
	g_kbetime = dicCmd.g_kbetime();

	EntityApp<Base>::onDbmgrInitCompleted(pChannel, ::time(0), dicCmd.startentityid(), dicCmd.endentityid(),
		0, 0, "ss");

	PyObject* pyResult = PyObject_CallMethod(getEntryScript().get(),
		const_cast<char*>("onAppInit"),
		const_cast<char*>("i"),
		0);

	if (pyResult != NULL)
		Py_DECREF(pyResult);
	else
		SCRIPT_ERROR_CHECK();

	pInitProgressHandler_ = new InitProgressHandler(this->networkInterface());
}

void BaseApp::onGetEntityAppFromDbmgr(Network::Channel* pChannel, MemoryStream& s)
{
	if (pChannel->isExternal())
		return;

	base_dbmgr::GetEntityAppFromDbmgr geafCmd;
	PARSEBUNDLE(s, geafCmd);

	Components::ComponentInfos* cinfos = Components::getSingleton().findComponent((
		KBEngine::COMPONENT_TYPE)geafCmd.componenttype(), geafCmd.componentid());

	DEBUG_MSG(fmt::format("BaseApp::onGetEntityAppFromDbmgr: app(uid:{0}, username:{1}, componentType:{2}, "
		"componentID:{3}\n",
		geafCmd.uid(),
		geafCmd.username(),
		COMPONENT_NAME_EX((COMPONENT_TYPE)geafCmd.componenttype()),
		geafCmd.componentid()
		));
	if (cinfos)
	{
		if (cinfos->pIntAddr->ip != geafCmd.intaddr() || cinfos->pIntAddr->port != geafCmd.intport())
		{
			ERROR_MSG(fmt::format("Baseapp::onGetEntityAppFromDbmgr: Illegal app(uid:{0}, username:{1}, componentType:{2}, "
				"componentID:{3}\n",
				geafCmd.uid(),
				geafCmd.username(),
				COMPONENT_NAME_EX((COMPONENT_TYPE)geafCmd.componenttype()),
				geafCmd.componentid()
				 ));
		}
	}

	Components::getSingleton().connectComponent((COMPONENT_TYPE)geafCmd.componenttype(),
		geafCmd.componentid(), geafCmd.intaddr(), geafCmd.intport());

	KBE_ASSERT(Components::getSingleton().getDbmgr() != NULL);
}

//-------------------------------------------------------------------------------------
void BaseApp::onClientHello(Network::Channel* pChannel, MemoryStream& s)
{
	client_baseserver::Hello helloCmd;
	PARSEBUNDLE(s, helloCmd)
	//	uint32 clientVersion = helloCmd.version();
	const std::string& extradata = helloCmd.extradata();

	Network::Bundle* pBundle = Network::Bundle::ObjPool().createObject();
	pBundle->newMessage(91, 2);

	client_baseserver::HelloCB helloCbCmd;
	helloCbCmd.set_result(1);
	helloCbCmd.set_version(68);
	helloCbCmd.set_extradata(extradata);

	ADDTOBUNDLE((*pBundle), helloCbCmd)
	pChannel->send(pBundle);
}

//-------------------------------------------------------------------------------------
void BaseApp::loginBaseapp(Network::Channel* pChannel, MemoryStream& s)
{
	client_baseserver::Login loginCmd;
	PARSEBUNDLE(s, loginCmd)
	std::string accountName = loginCmd.account();
	std::string password = loginCmd.password();
	accountName = KBEngine::strutil::kbe_trim(accountName);
	if (accountName.size() > ACCOUNT_NAME_MAX_LENGTH)
	{
		ERROR_MSG(fmt::format("Baseapp::loginBaseapp: accountName too big, size={}, limit={}.\n",
			accountName.size(), ACCOUNT_NAME_MAX_LENGTH));

		return;
	}

	if (password.size() > ACCOUNT_PASSWD_MAX_LENGTH)
	{
		ERROR_MSG(fmt::format("Baseapp::loginBaseapp: password too big, size={}, limit={}.\n",
			password.size(), ACCOUNT_PASSWD_MAX_LENGTH));

		return;
	}

	INFO_MSG(fmt::format("Baseapp::loginBaseapp: new user[{0}], channel[{1}].\n",
		accountName, pChannel->c_str()));

	Components::ComponentInfos* dbmgrinfos = Components::getSingleton().getDbmgr();
	if (dbmgrinfos == NULL || dbmgrinfos->pChannel == NULL || dbmgrinfos->cid == 0)
	{
		loginBaseappFailed(pChannel, accountName, SERVER_ERR_SRV_NO_READY);
		return;
	}

	PendingLoginMgr::PLInfos* ptinfos = pendingLoginMgr_.find(accountName);
	if (ptinfos == NULL)
	{
		loginBaseappFailed(pChannel, accountName, SERVER_ERR_ILLEGAL_LOGIN);
		return;
	}

	if (ptinfos->password != password)
	{
		loginBaseappFailed(pChannel, accountName, SERVER_ERR_PASSWORD);
		return;
	}

	if (idClient_.size() == 0)
	{
		ERROR_MSG("Baseapp::loginBaseapp: idClient size is 0.\n");
		loginBaseappFailed(pChannel, accountName, SERVER_ERR_SRV_NO_READY);
		return;
	}

	// 如果entityID大于0则说明此entity是存活状态登录
	if (ptinfos->entityID > 0)
	{
		INFO_MSG(fmt::format("Baseapp::loginBaseapp: user[{}] has entity({}).\n",
			accountName.c_str(), ptinfos->entityID));

		//Proxy* base = static_cast<Proxy*>(findEntity(ptinfos->entityID));
		//if (base == NULL || base->isDestroyed())
		//{
		//	loginBaseappFailed(pChannel, accountName, SERVER_ERR_BUSY);
		//	return;
		//}

		//// 通知脚本异常登录请求有脚本决定是否允许这个通道强制登录
		//int32 ret = base->onLogOnAttempt(pChannel->addr().ipAsString(),
		//	ntohs(pChannel->addr().port), password.c_str());

		//switch (ret)
		//{
		//case LOG_ON_ACCEPT:
		//	if (base->clientMailbox() != NULL)
		//	{
		//		// 通告在别处登录
		//		Network::Channel* pOldClientChannel = base->clientMailbox()->getChannel();
		//		if (pOldClientChannel != NULL)
		//		{
		//			INFO_MSG(fmt::format("Baseapp::loginBaseapp: script LOG_ON_ACCEPT. oldClientChannel={}\n",
		//				pOldClientChannel->c_str()));

		//			kickChannel(pOldClientChannel, SERVER_ERR_ACCOUNT_LOGIN_ANOTHER);
		//		}
		//		else
		//		{
		//			INFO_MSG("Baseapp::loginBaseapp: script LOG_ON_ACCEPT.\n");
		//		}

		//		base->clientMailbox()->addr(pChannel->addr());
		//		base->addr(pChannel->addr());
		//		base->setClientType(ptinfos->ctype);
		//		base->setClientDatas(ptinfos->datas);
		//		createClientProxies(base, true);
		//	}
		//	else
		//	{
		//		// 创建entity的客户端mailbox
		//		EntityMailbox* entityClientMailbox = new EntityMailbox(base->pScriptModule(),
		//			&pChannel->addr(), 0, base->id(), MAILBOX_TYPE_CLIENT);

		//		base->clientMailbox(entityClientMailbox);
		//		base->addr(pChannel->addr());
		//		base->setClientType(ptinfos->ctype);
		//		base->setClientDatas(ptinfos->datas);

		//		// 将通道代理的关系与该entity绑定， 在后面通信中可提供身份合法性识别
		//		entityClientMailbox->getChannel()->proxyID(base->id());
		//		createClientProxies(base, true);
		//	}
		//	break;
		//case LOG_ON_WAIT_FOR_DESTROY:
		//default:
			INFO_MSG("Baseapp::loginBaseapp: script LOG_ON_REJECT.\n");
			loginBaseappFailed(pChannel, accountName, SERVER_ERR_ACCOUNT_IS_ONLINE);
			return;
		//};
	}
	else
	{
		ENTITY_ID entityID = idClient_.alloc();
		KBE_ASSERT(entityID > 0);

		Network::Bundle* pBundle = Network::Bundle::ObjPool().createObject();
		(*pBundle).newMessage(DbmgrInterface::QueryAccount);

		base_dbmgr::QueryAccount queryCmd;
		queryCmd.set_account(accountName);
		queryCmd.set_password(password);
		queryCmd.set_componentid(g_componentID);
		queryCmd.set_entityid(entityID);
		queryCmd.set_entitydbid(ptinfos->entityDBID);
		queryCmd.set_addrip(pChannel->addr().ip);
		queryCmd.set_addrport(pChannel->addr().port);

		ADDTOBUNDLE((*pBundle), queryCmd)
		dbmgrinfos->pChannel->send(pBundle); //发送给dbmgr
	}

	// 记录客户端地址
	ptinfos->addr = pChannel->addr();
}

//
void BaseApp::onClientUpMsg(Network::Channel* pChannel, MemoryStream& s)
{
	ENTITY_ID pid = pChannel->proxyID();

	if (pid > 0)
	{
		Proxy* proxy = static_cast<Proxy*>(this->findEntity(pid));
		if (proxy)
		{
			proxy->OnProcessClientUpMsg(s);
			return;
		}
		else
		{
			ERROR_MSG(fmt::format("onClientUpMsg proxyid:{} not found!", pid));
		}
	}
	else
	{
		ERROR_MSG(fmt::format("onClientUpMsg error  proxyid is null!"));
	}
}

//-------------------------------------------------------------------------------------
void BaseApp::loginBaseappFailed(Network::Channel* pChannel, std::string& accountName,
	SERVER_ERROR_CODE failedcode, bool relogin)
{
	if (failedcode == SERVER_ERR_NAME)
	{
		DEBUG_MSG(fmt::format("Baseapp::login: not found user[{}], login is failed!\n",
			accountName.c_str()));

		failedcode = SERVER_ERR_NAME_PASSWORD;
	}
	else if (failedcode == SERVER_ERR_PASSWORD)
	{
		DEBUG_MSG(fmt::format("Baseapp::login: user[{}] password is error, login is failed!\n",
			accountName.c_str()));

		failedcode = SERVER_ERR_NAME_PASSWORD;
	}

	if (pChannel == NULL)
		return;

	Network::Bundle* pBundle = Network::Bundle::ObjPool().createObject();

	pBundle->newMessage(91, 4);

	client_baseserver::LoginBaseappFailed loginfailedCmd;
	loginfailedCmd.set_retcode(failedcode);

	ADDTOBUNDLE((*pBundle), loginfailedCmd)
	pChannel->send(pBundle);
}

void BaseApp::onQueryPlayerCBFromDbmgr(Network::Channel* pChannel, MemoryStream& s)
{
	if (pChannel->isExternal())
		return;

	base_dbmgr::QueryPlayerCBFromDbmgr queryCmd;
	PARSEBUNDLE(s, queryCmd);

	std::string accountName = queryCmd.account();
	std::string password = queryCmd.password();
	bool success = queryCmd.success() > 0;
	DBID dbid = queryCmd.entitydbid();
	ENTITY_ID entityID = queryCmd.entityid();

	PendingLoginMgr::PLInfos* ptinfos = pendingLoginMgr_.remove(accountName);
	if (ptinfos == NULL)
	{
		ERROR_MSG(fmt::format("Baseapp::onQueryAccountCBFromDbmgr: PendingLoginMgr not found({})\n",
			accountName.c_str()));
		return;
	}

	Network::Channel* pClientChannel = this->networkInterface().findChannel(ptinfos->addr);

	if (!success)
	{
		const std::string& error = queryCmd.datas();
		ERROR_MSG(fmt::format("Baseapp::onQueryAccountCBFromDbmgr: query {} is failed! error({})\n",
			accountName.c_str(), error));

		loginBaseappFailed(pClientChannel, accountName, SERVER_ERR_SRV_NO_READY);
		SAFE_RELEASE(ptinfos);
		return;
	}

	Proxy* base = static_cast<Proxy*>(createEntity("Player", entityID));

	if (!base)
	{
		ERROR_MSG(fmt::format("Baseapp::onQueryAccountCBFromDbmgr: create {} is failed! error(base == NULL)\n",
			accountName.c_str()));

		loginBaseappFailed(pClientChannel, accountName, SERVER_ERR_SRV_NO_READY);
		SAFE_RELEASE(ptinfos);
		return;
	}

	KBE_ASSERT(base != NULL);
	base->accountName(accountName);
	base->password(password);
	base->hasDB(true);
	base->dbid(dbid);
	base->setClientType(ptinfos->ctype);
	base->setLoginDatas(ptinfos->datas);
	base->InitDatas(queryCmd.datas());

	if (pClientChannel != NULL)
	{
		base->addr(pClientChannel->addr());

		createClientProxies(base);
		base->onUserLogonOn();
	}

	SAFE_RELEASE(ptinfos);
}

void BaseApp::onWriteToDBCallback(Network::Channel* pChannel, MemoryStream& s)
{
	if (pChannel->isExternal())
		return;

	base_dbmgr::WriteToDBCallback callbackCmd;
	PARSEBUNDLE(s, callbackCmd);

	int8 success = callbackCmd.success();
	DBID dbid = callbackCmd.entitydbid();
	ENTITY_ID entityID = callbackCmd.entityid();
	CALLBACK_ID callbackid = callbackCmd.callbackid();

	Base* base = pEntities_->find(entityID);
	if (base == NULL)
	{
		//ERROR_MSG(fmt::format("Baseapp::onWriteToDBCallback: can't found entity:{}.\n", entityID));
		return;
	}

	base->onWriteToDBCallback(entityID, dbid, callbackid, -1, success);

	S_RELEASE(base);
}

//-------------------------------------------------------------------------------------
bool BaseApp::createClientProxies(Proxy* base, bool reload)
{
	// 将通道代理的关系与该entity绑定， 在后面通信中可提供身份合法性识别
	Network::Channel* pChannel = base->pChannel();
	pChannel->proxyID(base->id());
	base->addr(pChannel->addr());

	// 重新生成一个ID
	if (reload)
		base->rndUUID(genUUID64());

	// 一些数据必须在实体创建后立即访问
	//base->initClientBasePropertys();

	// 让客户端知道已经创建了proxices, 并初始化一部分属性
	//Network::Bundle* pBundle = Network::Bundle::createPoolObject();
	//(*pBundle).newMessage(ClientInterface::onCreatedProxies);
	//(*pBundle) << base->rndUUID();
	//(*pBundle) << base->id();
	//(*pBundle) << base->ob_type->tp_name;
	//base->sendToClient(ClientInterface::onCreatedProxies, pBundle);
	Network::Bundle* pBundle = Network::Bundle::createPoolObject();
	pBundle->newMessage(91, 5);

	client_baseserver::CreatedProxies proxyCmd;
	proxyCmd.set_entityid(base->id());

	ADDTOBUNDLE((*pBundle), proxyCmd)
		base->sendToClient(pBundle);


	// 本应该由客户端告知已经创建好entity后调用这个接口。
	//if(!reload)
	base->onEntitiesEnabled();

	////发送登录成功协议
	//base->sendUserDownInfo();
	return true;
}


//-------------------------------------------------------------------------------------
}
