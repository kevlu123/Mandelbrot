# Usage

mandelbrot [-cpu|-gpu|-clcpu|-clgpu] [-sync] [-vsync]

## -cpu

Use the cpu to render. 

## -gpu

Use Direct3D11 on the gpu to render. This is by far the fastest option as it doesn't require the frame buffer to be transferred to the gpu every frame.

## -clcpu
Use OpenCL on the cpu to render. This requires the OpenCL runtime to be installed for the cpu.

## -clgpu (default)

Use OpenCL on the gpu to render. This requires the OpenCL runtime to be installed for the gpu. It is probably already installed if you have a dedicated gpu.

## -sync

This option is only used for -cpu. If present, it uses the main thread to render synchronously. Otherwise it uses n threads to render asynchronously where n is the number of logical processors on the current machine. 

## -vsync

Only present the screen buffer on vsync intervals. This caps the fps to the monitor refresh rate.

# Controls

Use the WASD keys to move the viewport and scroll to zoom.

# Screenshots

![Example 1](./screenshots/1.png)
![Example 2](./screenshots/2.png)
