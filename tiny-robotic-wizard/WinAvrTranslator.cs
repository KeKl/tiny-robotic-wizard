using System;
using System.Collections.Generic;
using System.IO;
using System.Diagnostics;
using System.Threading;
using System.Text;
using System.Xml;
using System.Xml.Xsl;
using System.Windows.Forms;

namespace tiny_robotic_wizard
{
    /// <summary>
    /// WinAVR��p���āC�Ώۂ̃v���O������AVR�Ŏ��s�\�Ȍ`���ɕϊ�����D
    /// </summary>
    public class WinAvrTranslator
    {
        private Dictionary<string, string> config;

        public WinAvrTranslator()
        {
            this.config = new Dictionary<string, string>();
            loadDefaultConfigurations();
        }

        /// <summary>
        /// �W���̐ݒ��ǂݍ���
        /// </summary>
        private void loadDefaultConfigurations()
        {
            this.config.Clear();
            this.config["WinAvrPath"] = @"WinAVR\bin";
            this.config["Frequency"] = "8000000UL";
            this.config["MCU"] = "atmega88";
            this.config["CFlags"] = "-Wall -gdwarf-2 -std=gnu99 -Os -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -MD -MP ";
            this.config["FlashFlags"] = "-O ihex -R .eeprom";
            this.config["EepFlags"] = "-O ihex -j .eeprom --set-section-flags=.eeprom=\"alloc,load\" --change-section-lma .eeprom=0 --no-change-warnings";
            this.config["IncludeDirs"] = "-I\".\"";
            this.config["ListOpts"] = "-h -S";
            this.config["CCompiler"] = "avr-gcc.exe";
            this.config["Make"] = "make.exe";
            this.config["ObjCopy"] = "avr-objcopy.exe";
            this.config["ObjDump"] = "avr-objdump.exe";
        }

        public void Translate(string input, Stream output)
        {
            // WinAVR\bin�̃t���p�X
            string winAvrPath = Path.Combine(Application.StartupPath, this.config["WinAvrPath"]);

            // ��Ɨp�̃f�B���N�g��������āC������C�t�@�C�������
            DirectoryInfo tempDirectory = Directory.CreateDirectory(Path.Combine(Application.StartupPath, "temp"));
            string tempPath = Path.Combine(tempDirectory.FullName, "program.c");
            StreamWriter program = new StreamWriter(tempPath);
            program.Write(input);
            program.Close();

            // �O���̎��s�t�@�C���̃G���[���b�Z�[�W
            string stdErrorContent;

            // �o�͂������ʂ�GCC��p���ăR���p�C������D
            string elfFileName = Path.ChangeExtension(tempPath, "elf");
            {
                // �����̐ݒ�
                StringBuilder args = new StringBuilder();
                args.AppendFormat("-o \"{0}\" ", Path.GetFileName(elfFileName));
                args.AppendFormat("-mmcu={0} ", this.config["MCU"]);
                args.AppendFormat("-DF_CPU={0} ", this.config["Frequency"]);
                args.AppendFormat("{0} ", this.config["CFlags"]);
                args.AppendFormat("\"{0}\"", Path.GetFileName(tempPath));

                // �R���p�C������
                string ccPath = Path.Combine(winAvrPath, this.config["CCompiler"]);
                if (this.ExecuteExternal(ccPath, tempDirectory.FullName, args.ToString(), out stdErrorContent) != 0)
                    throw new Exception("�R���p�C�����ɃG���[���������܂����D" + Environment.NewLine + stdErrorContent);
            }

            // ELF����Intel HEX�ɕϊ�����D
            string hexFileName = Path.ChangeExtension(tempPath, "hex");
            {
                // �����̐ݒ�
                StringBuilder args = new StringBuilder();
                args.AppendFormat("{0} \"{1}\" \"{2}\"", this.config["FlashFlags"], elfFileName, hexFileName);

                // HEX�t�@�C������
                string objcopyPath = Path.Combine(winAvrPath, this.config["ObjCopy"]);
                if (this.ExecuteExternal(objcopyPath, tempDirectory.FullName, args.ToString(), out stdErrorContent) != 0)
                    throw new Exception("�R���p�C�����ɃG���[���������܂����D" + Environment.NewLine + stdErrorContent);
            }
#if DEBUG
            // ���X�g�t�@�C���𐶐�����D
            string lstFileName = Path.ChangeExtension(tempPath, "lst");
            {
                // �����̐ݒ�
                StringBuilder args = new StringBuilder();
                args.AppendFormat("{0} \"{1}\"", this.config["ListOpts"], elfFileName);

                // ���X�g�t�@�C������
                string objdumpPath = Path.Combine(winAvrPath, this.config["ObjDump"]);
                string debugList;
                if (this.ExecuteExternal(objdumpPath, tempDirectory.FullName, args.ToString(), out debugList, out stdErrorContent) != 0)
                    throw new Exception("�R���p�C�����ɃG���[���������܂����D" + Environment.NewLine + stdErrorContent);

                // �t�@�C���̏����o��
                using (StreamWriter lstWriter = new StreamWriter(lstFileName))
                    lstWriter.Write(debugList);
            }
#endif
            // ���ʂ��o�͂̃X�g���[���ɃR�s�[���C���̃t�@�C�����폜����D
            {
                // 10k�m�ۂ���Α���邾�낤
                byte[] buffer = new byte[10480];
                using (FileStream resultFile = File.OpenRead(hexFileName))
                {
                    while (resultFile.CanRead)
                    {
                        int bytesRead = resultFile.Read(buffer, 0, buffer.Length);
                        if (bytesRead == 0) break;
                        output.Write(buffer, 0, bytesRead);
                    }
                }
            }
            // �X�g���[���̃|�C���^��擪�ɖ߂��D
            output.Seek(0, SeekOrigin.Begin);
        }

        private int ExecuteExternal(string path, string workDir, string args, out string error)
        {
            string output;
            return this.ExecuteExternal(path, workDir, args, out output, out error);
        }
        private int ExecuteExternal(string path, string workDir, string args, out string output, out string error)
        {
            ProcessStartInfo startInfo = new ProcessStartInfo(path);
            startInfo.WorkingDirectory = workDir;
            startInfo.Arguments = args;
            startInfo.CreateNoWindow = true;
            startInfo.UseShellExecute = false;
            startInfo.RedirectStandardOutput = true;
            startInfo.RedirectStandardError = true;
            Process process = Process.Start(startInfo);

            StringBuilder outputString = new StringBuilder();
            while (!process.HasExited)
            {
                outputString.Append(process.StandardOutput.ReadToEnd());
                Thread.Sleep(1);
            }
            outputString.Append(process.StandardOutput.ReadToEnd());

            error = process.StandardError.ReadToEnd();
            output = outputString.ToString();
            return process.ExitCode;
        }
    }
}
