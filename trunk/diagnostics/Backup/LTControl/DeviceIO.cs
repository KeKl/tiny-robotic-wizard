using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

using Microsoft.Win32.SafeHandles;

namespace DeviceIOLib
{
    /// <summary>
    /// AllocHGlobal/FreeHGlobal���g�����o�b�t�@�̃��b�p�[�N���X
    /// </summary>
    public class GlobalBuffer : IDisposable
    {
        private IntPtr ptr = IntPtr.Zero;
        private int size = 0;
        
        /// <summary>
        /// �o�b�t�@�̃|�C���^��Ԃ�
        /// </summary>
        public IntPtr Pointer
        {
            get { return ptr; }
        }
        /// <summary>
        /// �o�b�t�@�̃T�C�Y��Ԃ�
        /// </summary>
        public int Size
        {
            get { return size; }
        }
        /// <summary>
        /// IntPtr�ւ̕ϊ�
        /// </summary>
        /// <param name="buffer"></param>
        /// <returns></returns>
        public static implicit operator IntPtr(GlobalBuffer buffer)
        {
            return buffer.Pointer;
        }

        /// <summary>
        /// ���̃o�b�t�@�̓��e�̃R�s�[������byte�^�̔z���Ԃ��B
        /// </summary>
        /// <returns>���̃o�b�t�@�̓��e�̃R�s�[������byte�^�̔z��</returns>
        public byte[] ToByteArray()
        {
            byte[] buffer = new byte[size];
            Marshal.Copy(ptr, buffer, 0, size);
            return buffer;
        }

        public byte[] ToByteArray(byte[] destination, int start)
        {
            Marshal.Copy(ptr, destination, start, size);
            return destination;
        }


        /// <summary>
        /// byte�^�̔z����w�肳�ꂽ�I�t�Z�b�g�ʒu�ɃR�s�[����
        /// </summary>
        /// <param name="data">�R�s�[����z��</param>
        /// <param name="start">�z����̊J�n�ʒu</param>
        /// <param name="offset">�I�t�Z�b�g</param>
        public void WriteByteArray(byte[] data, int start, int offset, int length)
        {
            Marshal.Copy(data, start, (IntPtr)((int)ptr + offset), length);
        }

        public GlobalBuffer(int size)
        {
            ptr = Marshal.AllocHGlobal(size);
            this.size = size;
        }

        #region Dispose Pattern
        private bool disposed = false;
        public void Dispose()
        {
            Dispose(true);              //����I
            GC.SuppressFinalize(this);  //������������ŁBGC�N�B
        }
        ~GlobalBuffer()
        {
            Dispose(false);
        }
        private void Dispose(bool disposing)
        {

            if (!disposed)
            {
                if (disposing)
                {
                    if (ptr != null)
                    {
                        Marshal.FreeHGlobal(ptr);
                        ptr = IntPtr.Zero;
                    }
                }
                disposed = true;
            }
        }
        #endregion
    }

    public class DeviceIO
    {
        [DllImport("kernel32", SetLastError=true, CharSet = CharSet.Unicode)]
        public extern static SafeFileHandle CreateFile(string lpFileName, UInt32 dwDesiredAccess, UInt32 dwSharedMode, IntPtr lpSecurityAttribtues, UInt32 dwCreationDisposition, UInt32 dwFlagsAndAttributes, IntPtr hTemplateFile);
        [DllImport("kernel32", SetLastError=true)]
        public extern static bool DeviceIoControl(
          SafeFileHandle hDevice,
          UInt32 dwIoControlCode,
          IntPtr lpInBuffer,
          int nInBufferSize,
          IntPtr lpOutBuffer,
          int nOutBufferSize,
          out int lpBytesReturned,
          IntPtr lpOverlapped
        );

        [DllImport("kernel32")]
        public extern static bool ReadFile(SafeFileHandle hFile, IntPtr lpBuffer, int nNumberOfBytesToRead, out int lpNumberOfBytesRead, IntPtr lpOverlapped);
        [DllImport("kernel32")]
        public extern static bool WriteFile(SafeFileHandle hFile, IntPtr lpBuffer, int nNumberOfBytesToWrite, out int lpNumberOfBytesWrite, IntPtr lpOverlapped);

        public const UInt32 GENERIC_READ =(0x80000000U);
        public const UInt32 GENERIC_WRITE = (0x40000000U);
        public const UInt32 GENERIC_EXECUTE = (0x20000000U);
        public const UInt32 GENERIC_ALL = (0x10000000U);

        public const UInt32 FILE_SHARE_READ = 0x00000001U;
        public const UInt32 FILE_SHARE_WRITE = 0x00000002U;
        public const UInt32 FILE_SHARE_DELETE = 0x00000004U;

        public const UInt32 OPEN_EXISTING = 3;

        public const Int32 ERROR_ACCESS_DENIED = 5;

        public const UInt32 IOCTL_SCSI_BASE = 0x04U;
        public const UInt32 IOCTL_STORAGE_BASE = 0x0000002dU; //FILE_DEVICE_MASS_STORAGE
        public const UInt32 METHOD_BUFFERED = 0;
        public const UInt32 FILE_ANY_ACCESS = 0;
        public const UInt32 FILE_SPECIAL_ACCESS = 0;
        public const UInt32 FILE_READ_ACCESS = 1U;
        public const UInt32 FILE_WRITE_ACCESS = 2U;

        [StructLayout(LayoutKind.Sequential, Pack=1)]
        public struct SECURITY_ATTRIBUTES
        {
            public int nLength;
            public IntPtr lpSecurityDescriptor;
            public bool bInheritHandle;
        }

        /*
        #define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
            ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
        )
         */

    }

}
