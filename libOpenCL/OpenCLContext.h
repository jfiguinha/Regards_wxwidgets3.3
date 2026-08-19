#pragma once


namespace Regards
{
	namespace OpenCL
	{
        class COpenCLContext
        {
        public:
            COpenCLContext(){};
            ~COpenCLContext() = default;
            void Bind();
            void initializeContextFromGL();
            void AssociateToVulkan();
            void CreateDefaultOpenCLContext();
            void GetOutputData(cl_mem cl_output_buffer, void* dataOut, const int& sizeOutput, const int& flag);
            cv::ocl::Program GetProgram(const wxString& programName);
			cv::ocl::OpenCLExecutionContext GetExecutionContext() { return clExecCtx; }

        private:
            cl_command_queue s_queue;
			cv::ocl::OpenCLExecutionContext clExecCtx;
            cl_command_queue CreateCommandQueue(cl_command_queue_properties queue_properties = 0);
            wxString GetDeviceInfo(cl_device_id device, cl_device_info param_name);
            cl_device_id GetListOfDevice(cl_platform_id platform, cl_device_type device_type, int& found);
        };
    }
}