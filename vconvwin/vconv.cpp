#include "vconv.h"
#include <iostream>
#include <Windows.h>
#include <Xinput.h>

#include <ViGEm/Client.h>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "xinput.lib")

//用于接收手柄震动消息
VOID CALLBACK notification(
	PVIGEM_CLIENT Client,
	PVIGEM_TARGET Target,
	UCHAR LargeMotor,
	UCHAR SmallMotor,
	UCHAR LedNumber,
	LPVOID UserData
)
{
	static int count = 1;

	std::cout.width(3);
	std::cout << count++ << " ";
	std::cout.width(3);
	std::cout << (int)LargeMotor << " ";
	std::cout.width(3);
	std::cout << (int)SmallMotor << std::endl;
}

int VGMain()
{
	const auto client = vigem_alloc();

	if (client == nullptr)
	{
		std::cerr << "Uh, not enough memory to do that?!" << std::endl;
		return -1;
	}
	const auto retval = vigem_connect(client);

	if (!VIGEM_SUCCESS(retval))
	{
		std::cerr << "ViGEm Bus connection failed with error code: 0x" << std::hex << retval << std::endl;
		return -1;
	}

	//
	// Allocate handle to identify new pad
	//
	const auto pad = vigem_target_x360_alloc();

	//
	// Add client to the bus, this equals a plug-in event
	//
	const auto pir = vigem_target_add(client, pad);

	//
	// Error handling
	//
	if (!VIGEM_SUCCESS(pir))
	{
		std::cerr << "Target plugin failed with error code: 0x" << std::hex << pir << std::endl;
		return -1;
	}

	XINPUT_STATE state;

	//
	// Grab the input from a physical X360 pad in this example
	//
	XInputGetState(0, &state);

	//
	// The XINPUT_GAMEPAD structure is identical to the XUSB_REPORT structure
	// so we can simply take it "as-is" and cast it.
	//
	// Call this function on every input state change e.g. in a loop polling
	// another joystick or network device or thermometer or... you get the idea.
	//
	vigem_target_x360_update(client, pad, *reinterpret_cast<XUSB_REPORT*>(&state.Gamepad));

	//
	// We're done with this pad, free resources (this disconnects the virtual device)
	//
	vigem_target_remove(client, pad);
	vigem_target_free(pad);

	return 0;
}

PVIGEM_CLIENT client = NULL;
PVIGEM_TARGET targets[VCONV_MAX_CONTROLLER] = { NULL };

int VConVInit()
{
	client = vigem_alloc();
	if (client == nullptr)
		return -1;
	VIGEM_ERROR ret = vigem_connect(client);
	if (!VIGEM_SUCCESS(ret))
		return ret;
	return 0;
}

int VConVRelease()
{
	vigem_free(client);
	return 0;
}

int VConVConnectControllerAndListenPort(int n, WORD port)
{
	int r = VConVConnectController(n);
	if (r)
		return r;

	return VConVListenPort(n, port);
}

int VConVDisconnectControllerAndClosePort(int n)
{
	int r = VConVDisconnectController(n);
	if (r)
		return r;

	return VConVClosePort(n);
}

WORD VConVGetListeningPort(int n)
{
	//TODO
	return 0;
}

BOOL VConVGetControllerIsConnected(int n)
{
	return targets[n] != NULL;
}

int VConVConnectController(int n)
{
	targets[n] = vigem_target_x360_alloc();
	VIGEM_ERROR err = vigem_target_add(client, targets[n]);
	if (!VIGEM_SUCCESS(err))
	{
		vigem_target_remove(client, targets[n]);
		vigem_target_free(targets[n]);
		targets[n] = NULL;
		return err;
	}
	return 0;
}

int VConVDisconnectController(int n)
{
	VIGEM_ERROR err = vigem_target_remove(client, targets[n]);
	if (!VIGEM_SUCCESS(err))
		return err;
	return 0;
}

int VConVListenPort(int n, WORD port)
{
	//TODO
	return 0;
}

int VConVClosePort(int n)
{
	//TODO
	return 0;
}
