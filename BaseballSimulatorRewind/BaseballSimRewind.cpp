//Flock of Bird Headers
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <wincon.h>
#include "bird.h"
#include <string.h>

//SensoDuino Headers
#using <System.dll>

using namespace System;
using namespace System::IO::Ports;
using namespace System::Threading;

//IRRKlang Headers
#include <irrKlang.h>

//IrrPhysx Headers
#include <irrlicht.h>
#include <IrrPhysx.h>
#include "ShapeCreation.h"
#include "CGUIParamChanger.h"

#ifdef _IRR_WINDOWS_
#pragma comment(lib, "Irrlicht.lib")
//#pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup")
#endif

//IrrPhysx
using namespace irr;
using namespace scene;
using namespace IrrPhysx;

//IrrKlang
using namespace irrklang;
#pragma comment(lib, "irrKlang.lib")

//Other
#define PI  (3.141592653589793)
#define Gravity	-9.81f
double pos_x,pos_y,pos_z;
double ang_x,ang_y,ang_z;
double az,el,ro;
double el_old;
bool lvl_easy = false;
bool bat_swing = false;

//Screen Resolution
u32 screen_w = 1024;
u32 screen_h = 768;
u32 screen_bit = 32;
bool screen_full = false;
bool screen_aa = true;

//Screen Perspective
f32 screen_width = 3.0f;	// 3 meter
f32 screen_height = 2.0f;   // 2 meter
f32 screen_dist = 2.0f;		// 2 meter
f32 screen_height_offset = 1.0f; // 1meter

//Human Perspective
f32 human_height = 1.72f;   // 1.72 meter
f32 human_midpoint = human_height*0.72f;
f32 human_eye = human_height*0.936f;
f32 human_knee = human_height*0.285f;

//Strike Zone
f32 sz_upper_h = human_midpoint;
f32 sz_lower_h = human_knee;
f32 sz_width = 0.45f;
f32 sz_desired_h = (sz_upper_h+sz_lower_h)/2;

//Stadium
core::vector3df stadium_pos(0,0,0);
core::vector3df stadium_rot(0,45,0);
core::vector3df stadium_scale(1,1,1);

//Bat
core::vector3df bat_pos(0.4,human_midpoint,0.7);
core::vector3df bat_rot(-90,0,90);
core::vector3df bat_scale(1,1,1);
core::vector3df bat_scale2(10,1.75,11);
f32 bat_density = 600.0f;

//Camera
core::vector3df camera_pos(0,human_midpoint,0);
core::vector3df camera_target(18,1.7,0);
core::vector3df camera_pos_far(-2,1.7,0);
f32 camera_fov = 60*PI/180.0f;
f32 camera_far = 5000.0f;
f32 camera_near = 1.0f;

//Baseball
core::vector3df ball_ipos(18,human_midpoint,0);
core::vector3df ball_fpos(0,5.2f,0);
core::vector3df ball_fpos2(0,5.3f,0);
f32 ball_vel = 20.15f;
f32 ball_radius = 0.05f;
f32 ball_density = 1000.0f;

//Parallax Motor Drive via Serial Port
HANDLE hSerial;
LPCSTR motor_COM_port = "COM5";

//Flock of Bird Settings
#define GROUP_ID			1		// arbitrary designation for group
#define READ_TIMEOUT		1000	// 2000 mSec (2 seconds)
#define WRITE_TIMEOUT		1000	// 2000 mSec (2 seconds)
#define BAUD_RATE			115200	// 115.2K baud
BIRDFRAME frame;					// Holds new frames of data
double pos[3],ang[3];				// Array for P+O
WORD	COM_port = 4;				// COM PORT

// Declaration
IrrlichtDevice* device = NULL;
scene::ISceneManager* smgr = NULL;
video::IVideoDriver* driver = NULL;
gui::IGUIEnvironment* guienv = NULL;
scene::ICameraSceneNode* camera = NULL;
IPhysxManager* physxManager = NULL;
core::array<SPhysxAndNodePair*> objects;
f32 objectDensity = 50;
CGUIParamChanger* paramChanger = NULL;
// IrrKlang
ISoundEngine* soundengine = NULL;
// Bat
IMesh* meshbat;
SPhysxAndNodePair* pair_bat;
IPhysxMesh* physxMesh_bat;


public ref class SensoDuino
{
private:
    static SerialPort^ _serialPort;

public:
    static void openPort(){

	// Create a new SerialPort object with default settings.
        _serialPort = gcnew SerialPort();

       // These are the Arduino com defaults.
        _serialPort->PortName = "COM5";
        _serialPort->BaudRate = 115200;
        _serialPort->Parity = Parity::None;
        _serialPort->DataBits = 8;
        _serialPort->StopBits = StopBits::One;
        _serialPort->Handshake = Handshake::None;

        // Set the read/write timeouts
        _serialPort->ReadTimeout = 500;
        _serialPort->WriteTimeout = 500;

        _serialPort->Open();
	}



    static String^ Read()
    {
        
            try
            {
                String^ message = _serialPort->ReadLine();
				return message;
               // Console::WriteLine(message);
            }
            catch (TimeoutException ^) { }
        
    }

	static void closePort(){
		_serialPort->Close();
	}

	static array<double>^ getOrientation(){
		String^ delimStr = ",";
		array<Char>^ delimiter = delimStr->ToCharArray( );
		array<String^>^ words;
		String^ line = _serialPort->ReadLine();

		words = line->Split( delimiter );
		array<double>^ foo = gcnew array<double>(3);
			if(words->Length==5)
				{
					foo[0] = System::Convert::ToDouble(words[2]);
					foo[1] = System::Convert::ToDouble(words[3]);
					foo[2] = System::Convert::ToDouble(words[4]);
				}
		return foo;
	} 
};
    











// Flock of Bird
void birdInit ()
{
	SensoDuino::openPort();
//BIRDSYSTEMCONFIG sysconfig;		// Holds System configuration
//BIRDDEVICECONFIG devconfig[1];	// Holds Bird configuration
//
//printf("\n---------- Flock Of Birds ---------- \n\n");
//printf("Initializing\n");
//
//if	((!birdRS232WakeUp(
//	GROUP_ID,					// arbitrary designation for group
//	TRUE,						// Stand_alone mode
//	1,							// Number of Devices
//	&COM_port,					// Com Port
//	BAUD_RATE,					// BAUD
//	READ_TIMEOUT,WRITE_TIMEOUT,	// Reponse timeouts
//	GMS_GROUP_MODE_NEVER)))		// No group mode in Stand-alone mode
//	{
//		printf("Can't Wake Up Flock!\n");
//		Sleep(1000);
//	}
//	
//// Read system configuration data from bird into sysconfig structure	
//if (!birdGetSystemConfig(
//	GROUP_ID,	// arbitrary designation for group
//	&sysconfig,	// Structure for System configuration
//	TRUE))		// Yes, use copy of System Config from Wake Up command
//	{
//		printf("%s\n",birdGetErrorMessage());
//	}
//		
//// Read the device configuration into the devconfig structure
//if (!birdGetFastDeviceConfig(GROUP_ID,1,&devconfig[0]))
//	{
//		printf("%s\n",birdGetErrorMessage());
//		printf("Couldn't get device configuration for bird!\n");
//	}
//
//	// Start getting data
//	birdStartFrameStream(GROUP_ID);
	return;	
};

void birdRun()
{
	array<double>^ sensorData = gcnew array<double>(3);
	sensorData = SensoDuino::getOrientation();
	ang[0] = sensorData[0];
	ang[1] = sensorData[1];
	ang[2] = sensorData[2];
	//if(birdFrameReady(GROUP_ID))
	//{
	//	// Reads data from bird
	//	birdGetMostRecentFrame(GROUP_ID,&frame);
	//	BIRDREADING *bird_data; 	// Moves data into structure
	//	bird_data = &frame.reading[0]; // Sets pointer to bird 0
	//		
	//	// Convert data into inches and degrees and scale
	//	pos[0] = bird_data->position.nX * 36 / 32767. * 0.0254;
	//	pos[1] = bird_data->position.nY * 36 / 32767. * 0.0254;
	//	pos[2] = bird_data->position.nZ * 36 / 32767. * 0.0254;
	//	ang[0] = bird_data->angles.nAzimuth * 180. / 32767.;
	//	ang[1] = bird_data->angles.nElevation * 180. / 32767.;
	//	ang[2] = bird_data->angles.nRoll * 180. / 32767.;
	//		
	//	// print data
	//	//printf("%+6.1f  %+6.1f  %+6.1f  \n",pos[0], pos[1], pos[2]); 
	//	//printf("%+6.1f  %+6.1f  %+6.1f  \n",ang[0], ang[1], ang[2]);
	//	//printf("%+6.1f  %+6.1f  %+6.1f  \n",quat[0], quat[1], quat[2], quat[3]);

	//	} // end if frame ready routine
		return;
};

void birdEnd ()
{
	SensoDuino::closePort();
	/*printf("Flock of Bird: EXITING... \n");
	birdStopFrameStream(GROUP_ID);
 	birdShutDown(GROUP_ID);*/
	return;
};

// Sound
void soundInit(){
	printf("\n---------- Sound (IrrKlang) ---------- \n\n");
	printf("Initializing\n");
	soundengine = createIrrKlangDevice();
	if (!soundengine){
		// error starting up the engine
		printf("Sound: Could not startup engine\n");
	}
}
// Pulse Motor
void motorInit(){	
		/*hSerial = CreateFile(motor_COM_port, GENERIC_READ | GENERIC_WRITE,0,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
		printf("\n---------- Pulse Motor ---------- \n\n");
		if(hSerial==INVALID_HANDLE_VALUE){
			if(GetLastError()==ERROR_FILE_NOT_FOUND){
				printf("Pulse Motor: Serial Port does not exist\n");
			}
			else{
				printf("Pulse Motor: Unknown Error\n");
			}
		}*/
}

void motorRun(){
		/*char szBuff[11] = {88};
		DWORD dwBytesRead = 0;
		if(!WriteFile(hSerial, szBuff, 2, &dwBytesRead, NULL)){
			printf("Pulse Motor: Error Sending Command\n");}
		else{
			printf("Pulse Motor: PULSE\n");
		}*/
};

void motorEnd(){
	/*printf("Pulse Motor: EXITING... \n");
	CloseHandle(hSerial);*/
};

void batCreate()
{
	meshbat = smgr->getMesh("C:/Users/dIPEN/Documents/Visual Studio 2010/Projects/BaseballSimulatorRewind/media/models/bat/bat.3ds");


	pair_bat = new SPhysxAndNodePair;
	physxMesh_bat = physxManager->createConvexMesh(meshbat->getMeshBuffer(0), bat_scale2);
	pair_bat->PhysxObject = physxManager->createConvexMeshObject(physxMesh_bat, bat_pos, bat_rot, 1);
	pair_bat->SceneNode = smgr->addMeshSceneNode(meshbat, NULL, -1, bat_pos, bat_rot, bat_scale);

	pair_bat->SceneNode->setMaterialFlag(video::EMF_NORMALIZE_NORMALS, true);
	pair_bat->SceneNode->setMaterialFlag(video::EMF_LIGHTING, true);
	objects.push_back(pair_bat);
}

void batUpdate(){
	pos_x=-pos[0];	//-pos[0]  0
	pos_y=-pos[2];	//-pos[2]  3
	pos_z=pos[1];	//-pos[1]  4

	az=90-ang[1];
	el=ang[0]-90;
	ro=0;

	//printf("%+6.1f \n",el);

	//printf("%+6.1f \n",abs(el_old-el));
	if(abs(el_old-el)>20)
	{
		if(!bat_swing){
			soundengine->play2D("C:/Users/dIPEN/Documents/Visual Studio 2010/Projects/BaseballSimulatorRewind/media/media/batswing.mp3");
		}
		bat_swing = true;
	}
	else
	{
		bat_swing = false;
	}

	if(lvl_easy){
		//az=90-abs(180+el)/3;
		az=105+((cos((el+180)*PI/180)-1)*40);
	}

	//pair_bat->SceneNode->setPosition(core::vector3df(pos_x,pos_y,pos_z));
	pair_bat->SceneNode->setPosition(bat_pos);
	pair_bat->PhysxObject->movePosition(pair_bat->SceneNode->getPosition());
	pair_bat->SceneNode->setRotation(core::vector3df(az,el,ro));
	//pair_bat->SceneNode->setRotation(bat_rot);
	pair_bat->PhysxObject->moveRotation(pair_bat->SceneNode->getRotation());

	el_old=el;

}

void stadiumCreate(){
	scene::IMesh* mesh = smgr->getMesh("C:/Users/dIPEN/Documents/Visual Studio 2010/Projects/BaseballSimulatorRewind/media/models/stadium/stadium.3ds");	
	SPhysxAndNodePair* pair = new SPhysxAndNodePair;
	IPhysxMesh* physxMesh = physxManager->createTriangleMesh(mesh->getMeshBuffer(0), stadium_scale);
	pair->PhysxObject = physxManager->createTriangleMeshObject(physxMesh, stadium_pos, stadium_rot);
	pair->SceneNode = smgr->addMeshSceneNode(mesh, NULL, -1, stadium_pos, stadium_rot, stadium_scale);
	pair->SceneNode->setMaterialFlag(video::EMF_NORMALIZE_NORMALS, true);
	pair->SceneNode->setMaterialFlag(video::EMF_LIGHTING, true);
	objects.push_back(pair);
}

void skyboxCreate(){
		smgr->addSkyBoxSceneNode(
                driver->getTexture("C:/Users/dIPEN/Documents/Visual Studio 2010/Projects/BaseballSimulatorRewind/media/skybox/irrlicht2_up.jpg"),
                driver->getTexture("C:/Users/dIPEN/Documents/Visual Studio 2010/Projects/BaseballSimulatorRewind/media/skybox/irrlicht2_dn.jpg"),
                driver->getTexture("C:/Users/dIPEN/Documents/Visual Studio 2010/Projects/BaseballSimulatorRewind/media/skybox/irrlicht2_lf.jpg"),
                driver->getTexture("C:/Users/dIPEN/Documents/Visual Studio 2010/Projects/BaseballSimulatorRewind/media/skybox/irrlicht2_rt.jpg"),
                driver->getTexture("C:/Users/dIPEN/Documents/Visual Studio 2010/Projects/BaseballSimulatorRewind/media/skybox/irrlicht2_ft.jpg"),
                driver->getTexture("C:/Users/dIPEN/Documents/Visual Studio 2010/Projects/BaseballSimulatorRewind/media/skybox/irrlicht2_bk.jpg"));
}

// Baseball Create
SPhysxAndNodePair* createThrownBaseball(IPhysxManager* physxManager, scene::ISceneManager* smgr, video::IVideoDriver* driver, scene::ICameraSceneNode* camera) {
	core::vector3df vel = ball_fpos2 - ball_ipos;
	vel.normalize();
	if(lvl_easy){
		vel*=ball_vel;}
	else{
		vel*=ball_vel + (rand() % 20)/15.0f;
	}

	return createSphere(physxManager, smgr, driver, ball_ipos, ball_radius, ball_density, &vel);
}

// Key Press
class CEventReceiver : public IEventReceiver {
	public:
		bool OnEvent(const SEvent& event) {
			if (event.EventType == EET_KEY_INPUT_EVENT) {			
				if (event.KeyInput.PressedDown) return false;
				switch (event.KeyInput.Key) {
					case KEY_SPACE:
						// Throw a sphere from the camera
						//if(lvl_easy){
							ball_fpos2=ball_fpos;//}
						//else{
						//	ball_fpos2=core::vector3df(ball_fpos.X,ball_fpos.Y,ball_fpos.Z-0.25+( rand() % 10 )/40.0f);}
						objects.push_back(createThrownBaseball(physxManager, smgr, driver, camera));
						return true;
					case KEY_KEY_E:
						lvl_easy = !lvl_easy;
						return true;
					case KEY_KEY_V:
						// Toggle the debug data visibility
						physxManager->setDebugDataVisible(!physxManager->isDebugDataVisible());
						return true;
					case KEY_KEY_Q: 
						// Toggle the mouse controlling the camera on and off
						camera->setInputReceiverEnabled(!camera->isInputReceiverEnabled());
						device->getCursorControl()->setVisible(!device->getCursorControl()->isVisible());
						return true;
					case KEY_KEY_C:
						camera->setTarget(camera_target);
						camera->setPosition(camera_pos);
						camera->setTarget(camera_target);
						return true;
					case KEY_KEY_X:
						camera->setTarget(camera_target);
						camera->setPosition(camera_pos_far);
						camera->setTarget(camera_target);
						return true;
					case KEY_KEY_Z:
						camera->setPosition(core::vector3df(9,1.7,-10));
						camera->setTarget(core::vector3df(9,1.7,0));
						camera->setPosition(core::vector3df(9,1.7,-10));
						return true;
					case KEY_KEY_A:
						bat_pos=core::vector3df(0.0,human_midpoint,0.45);
						bat_rot=core::vector3df(0,45,45);
						return true;
					case KEY_KEY_S:
						bat_pos=core::vector3df(0.2,human_midpoint,0.45);
						bat_rot=core::vector3df(0,45,45);
						return true;
					case KEY_ESCAPE:
						// Close the device, and hence exit the game
						device->closeDevice();
						return true;
				}
			}
			return false;
		}
};

// Collision Detection Callback
void contactCallback(IPhysxObject* objectA, IPhysxObject* objectB) {
	// Collision of Sphere with Convex Mesh
	if (objectA->getType() == EOT_SPHERE && objectB->getType() == EOT_CONVEX_MESH) {
		printf("Bat & Ball Collision\n");					
		soundengine->play2D("C:/Users/dIPEN/Documents/Visual Studio 2010/Projects/BaseballSimulatorRewind/media/media/bat.mp3");
		motorRun();
	}
	// Collision of Sphere with Capsule
	if (objectA->getType() == EOT_SPHERE && objectB->getType() == EOT_CAPSULE) {
		printf("Bat & Ball Collision\n");					
		soundengine->play2D("C:/Users/dIPEN/Documents/Visual Studio 2010/Projects/BaseballSimulatorRewind/media/media/bat.mp3");
		motorRun();
	}
}

//Anaglyph

// Main Program
int main() {

	// Start Sound/Flock-of-Bird/Motor
	soundInit();
	//birdInit();
	motorInit();
	motorRun();	// Test Pulse

	//IrrLicht Device Setup
	CEventReceiver receiver;
	irr::SIrrlichtCreationParameters params;
	params.DriverType=video::EDT_OPENGL;
	//params.DriverType=video::EDT_DIRECT3D9;
	params.WindowSize=core::dimension2d<s32>(screen_w, screen_h);
	params.Stencilbuffer=true;
	params.AntiAlias=screen_aa;
	params.Fullscreen=screen_full;
	params.Vsync=false;
	params.EventReceiver=&receiver;
	
	printf("\n---------- IrrPhysx ---------- \n\n");
	// Create our Irrlicht device
	device = createDeviceEx(params);
	if (!device) {
		printf("failed irrlicht device creation\n");
		return 1;
	}

	// Set the window's title
	wchar_t wstr[50];
	swprintf(wstr, 50, L"MAE574 Final Project :: Baseball Simulator", IRRPHYSX_VERSION);
	device->setWindowCaption(wstr);

	// Grab some useful pointers
	smgr = device->getSceneManager();
	driver = device->getVideoDriver();
	guienv = device->getGUIEnvironment();

	// Create an FPS camera so we can see the scene and easily move around
	camera = smgr->addCameraSceneNodeFPS();
	camera->setInputReceiverEnabled(!camera->isInputReceiverEnabled());
	camera->setPosition(camera_pos);
	camera->setTarget(camera_target);
	camera->setFOV(camera_fov);
	camera->setFarValue(camera_far);
	camera->setNearValue(camera_near);
	device->getCursorControl()->setVisible(false);

	// Add a light to make things look a bit nicer
	scene::ILightSceneNode* light = smgr->addLightSceneNode(0, core::vector3df(0,5000,0), video::SColorf(1,1,1,1), 10000);
	smgr->setAmbientLight(video::SColorf(0,0,0,1));

	// Add a billboard to show the light's position
	scene::IBillboardSceneNode* bill = smgr->addBillboardSceneNode(light, core::dimension2d<f32>(25,25));
	bill->setMaterialTexture(0, driver->getTexture("C:/Users/dIPEN/Documents/Visual Studio 2010/Projects/BaseballSimulatorRewind/media/media/light.jpg"));
	bill->setMaterialType(video::EMT_TRANSPARENT_ADD_COLOR);
	bill->setMaterialFlag(video::EMF_LIGHTING, false);

	// Create our Physx manager which will help us deal with all the physics stuff
	SSceneDesc sceneDesc;
	physxManager = createPhysxManager(device, sceneDesc);

	// Set our contact callback function so the physx manager can inform us of contacts between objects
	physxManager->setContactCallback(&contactCallback);

	// Add a floor at the origin, horizontal
	addPlane(physxManager, smgr, 0, core::vector3df(0,10,0));

	// Create Objects
	skyboxCreate();
	batCreate();
	stadiumCreate();

	// Play Ambient Sound
	soundengine->play2D("C:/Users/dIPEN/Documents/Visual Studio 2010/Projects/BaseballSimulatorRewind/media/media/park.wav", true);

	// Add a nice font and some useful gui texts
	guienv->getSkin()->setFont(guienv->getFont("C:/Users/dIPEN/Documents/Visual Studio 2010/Projects/BaseballSimulatorRewind/media/media/GillSans12.png"));
	guienv->addImage(core::rect<s32>(10,10,215,240));
	gui::IGUIStaticText* fpsText = guienv->addStaticText(L"FPS: 0", core::rect<s32>(15,10,200,200));
	fpsText->setOverrideColor(video::SColor(255,255,255,255));
	gui::IGUIStaticText* numObjText = guienv->addStaticText(L"Num Objects: 0", core::rect<s32>(15,30,200,200));
	numObjText->setOverrideColor(video::SColor(255,255,255,255));
	gui::IGUIStaticText* generalText = guienv->addStaticText(L"Cursors: Move\nMouse: Look\nQ: Toggle Mouse For Camera\nSPACE: Throw Baseball\n\nV: Toggle debug data visible\nESC: Quit", core::rect<s32>(15,50,250,350));
	generalText->setOverrideColor(video::SColor(255,255,255,255));
	guienv->addImage(driver->getTexture("C:/Users/dIPEN/Documents/Visual Studio 2010/Projects/BaseballSimulatorRewind/media/media/irrlichtirrphysxlogo.png"), core::position2d<s32>(screen_w-150,screen_h-120));
	guienv->addImage(driver->getTexture("C:/Users/dIPEN/Documents/Visual Studio 2010/Projects/BaseballSimulatorRewind/media/media/nvidiaphysxlogo.png"), core::position2d<s32>(25,screen_h-75));
		
	s32 lastTime = device->getTimer()->getTime();

	// We're ready to render now!
	while (device->run()) {
		if (device->isWindowActive()) {
			// Calculate the elapsed time since the last frame
			s32 timeNow = device->getTimer()->getTime();
			s32 elapsedTime = timeNow - lastTime;
			lastTime = timeNow;

			// Set the physics simulation going
			physxManager->simulate(elapsedTime/1000.0f);
			//physxManager->simulate(1.0f/100.0f);

			// Whilst we're waiting we can do some non-physics related work
			// you must NEVER add/move physics objects whilst the simulation
			// is being done though, doing so will cause all kinds of horrible crashes!
			core::stringw strw;
			strw = "FPS: ";
			strw += driver->getFPS();
			fpsText->setText(strw.c_str());
			strw = "Num Objects: ";
			strw += physxManager->getNumPhysxObjects();
			numObjText->setText(strw.c_str());

			// Wait for the physics simulation to finish
			physxManager->fetchResults();
			
			// Update our objects to the correct transformations
			for (u32 i = 0 ; i < objects.size() ; i++) 
				objects[i]->updateTransformation();

			// Begin the render process
			driver->beginScene(true, true, video::SColor(225,120,120,120));
			
			// Render the scene
			smgr->drawAll();

			// Flock of Bird: Get current position/rotation & Update Bat Position
			//birdRun();
			//batUpdate();

			// Render Physx debug data
			physxManager->renderDebugData(video::SColor(225,255,255,255));
			// Render the GUI
			guienv->drawAll();
			// Finish the render process
			driver->endScene();
			
		} else device->yield();

	}
	
	// Clean up the physx manager
	removePhysxManager(physxManager);
	physxManager = 0;

	// Clean up the device
	device->drop();
	device = 0;

	// And we're done.
    return 0;

	// Stop Motor/Flock-of-Bird
	motorEnd();
	//birdEnd();

}
