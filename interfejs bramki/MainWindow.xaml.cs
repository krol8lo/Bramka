using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.IO.Ports;

namespace interface_bramki
{
    using Extensions;

    public partial class MainWindow : Window
    {
        public bool is_connected;
        public bool serial_data_recived;
        public bool device_connected;
        public bool timer_start;
        public bool timer_stop;
        public bool reset;

        public byte recived_byte;

        public int seconds = 0;
        public int miliseconds = 0;
        public int buff_seconds = 0;
        public int buff_miliseconds = 0;

        public string port_name = "";
        public string buff = "";

        public string[] ports = null;
        public string[] ports_buff = null;

        public Thread serial_status_update;
        public Thread other_stuff;

        public TimeSpan starting_time;
        public TimeSpan time;

        public SerialPort serial = new SerialPort();

        public MainWindow()
        {
            serial.BaudRate = 250000;
            serial.DataBits = 8;
            serial.StopBits = (StopBits)1;
            serial.WriteTimeout = 100;
            serial.ReadTimeout = 100;
            serial.DataReceived += new SerialDataReceivedEventHandler(DataRecivedHandler);
            serial.ErrorReceived += new SerialErrorReceivedEventHandler(ErrorRecivedHandler);

            InitializeComponent();

            serial_status_update = new Thread(() =>
            {
                ports = SerialPort.GetPortNames();

                Ports.Dispatcher.Invoke(() =>
                {
                    Ports.Items.Clear();

                    foreach (string str in ports)
                    {
                        Ports.Items.Add(str);
                    }
                });

                while (true)
                {
                    Thread.Sleep(10);
                    ports_buff = ports;
                    ports = SerialPort.GetPortNames();
                    if(timer_start)
                    {
                        buff_miliseconds = (DateTime.Now.TimeOfDay - starting_time).Milliseconds;
                        buff_seconds = (DateTime.Now.TimeOfDay - starting_time).Seconds;
                    }   
                    else if (timer_stop)
                    {
                        buff_miliseconds = time.Milliseconds;
                        buff_seconds = time.Seconds;
                        timer_stop = false;
                    }
                    else if(reset)
                    {
                        buff_miliseconds = 0;
                        buff_seconds = 0;
                        reset = false;
                    }

                    TimeTextBox.Dispatcher.Invoke(() =>
                    {
                        if (buff_seconds < 10) buff += "0" + buff_seconds.ToString();
                        else buff = buff_seconds.ToString();
                        if (buff_miliseconds < 10) buff += ",00" + buff_miliseconds.ToString();
                        else if (buff_miliseconds < 100) buff += ",0" + buff_miliseconds.ToString();
                        else buff += "," + buff_miliseconds.ToString();
                        TimeTextBox.Text = buff;
                        buff = "";
                    });

                    if (!CompareArraySequence(ports, ports_buff))
                    {
                        if (device_connected && !serial.IsOpen)
                        {
                            ConnectButton.Dispatcher.Invoke(() =>
                            {
                                ConnectButton.IsEnabled = true;
                            });
                            ResetButton.Dispatcher.Invoke(() =>
                            {
                                ResetButton.IsEnabled = false;
                            });
                            Ports.Dispatcher.Invoke(() =>
                            {
                                Ports.IsEnabled = true;
                            });
                            device_connected = false;
                        }


                        Ports.Dispatcher.Invoke(() =>
                        {
                            Ports.Items.Clear();

                            foreach (string str in ports)
                            {
                                Ports.Items.Add(str);
                            }
                        });
                    }
                }
            })
            { IsBackground = true };
            serial_status_update.Start();
        }

        ///////////////////////////////////////////////////////////////////////////////////////////////
        ///                                      Events Handlers                                    ///
        ///////////////////////////////////////////////////////////////////////////////////////////////

        private void DataRecivedHandler(object sender, SerialDataReceivedEventArgs e)
        {
            SerialPort sp = (SerialPort)sender;
            byte indata = (byte)sp.ReadByte();
            recived_byte = indata;

            if (recived_byte == (byte)'s')
            {
                starting_time = DateTime.Now.TimeOfDay;
                timer_start = true;
                recived_byte = 0;
            }
            if (recived_byte == (byte)'f')
            {
                time = DateTime.Now.TimeOfDay - starting_time;
                timer_start = false;
                timer_stop = true;
                recived_byte = 0;
            }

        }

        /*
        private static void SerialPinChangedEventHandler(object sender, SerialPinChangedEventArgs e)
        {
            SerialPort sp = (SerialPort)sender;
            MessageBox.Show(sp.)
        }
        */
        private void ErrorRecivedHandler(object sender, SerialErrorReceivedEventArgs e)
        {
            MessageBox.Show("kek");
        }

        private void TimeEventHandler(object sender, EventArgs e)
        {
            miliseconds++;
            if (miliseconds == 1000)
            {
                miliseconds = 0;
                seconds++;
            }
            buff_miliseconds = miliseconds;
            buff_seconds = seconds;
        }

        ///////////////////////////////////////////////////////////////////////////////////////////////
        ///                                         Stuff                                           ///
        ///////////////////////////////////////////////////////////////////////////////////////////////

        private void PortsChanged(object sender, RoutedEventArgs e)
        {
            int index;

            if (device_connected && (string)Ports.SelectedValue != port_name)
            {
                MessageBox.Show("Inne urządzenie jest podłączne, nie można zmienić", "Error", MessageBoxButton.OK, MessageBoxImage.Error);

                try
                {
                    Ports.SelectedValue = port_name;
                }
                catch
                {
                    MessageBox.Show("Coś poszło nie tak", "Error", MessageBoxButton.OK, MessageBoxImage.Error);

                    Ports.SelectedValue = null;
                    port_name = null;
                    device_connected = false;
                }

            }
            else
            {
                if (Ports.SelectedValue != null)
                {
                    index = Ports.SelectedValue.ToString().IndexOf("COM");
                    port_name = Ports.SelectedValue.ToString().Substring(index);
                }
            }
        }

        private void CB_Clicked(object sender, RoutedEventArgs e)
        {
            try
            {
                serial.PortName = port_name;
                serial.Open();
                device_connected = true;
                Ports.IsEnabled = false;
                ResetButton.IsEnabled = true;
            }
            catch
            {
                MessageBox.Show("Coś poszło nie tak", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }

        }

        private void DB_Clicked(object sender, RoutedEventArgs e)
        {
            serial.Close();
            device_connected = false;
            Ports.IsEnabled = true;
            ResetButton.IsEnabled = true;
        }

        private void RST_Clicked(object sender, RoutedEventArgs e)
        {
            reset = true;
            if ((string)ResetButton.Content == "start") ResetButton.Content = "reset";
            serial.WriteByte((byte)'r');
            miliseconds = 0;
            buff_miliseconds = 0;
            seconds = 0;
            buff_seconds = 0;
        }

        public bool CompareArraySequence(string[] str1, string[] str2)
        {
            if (str1 == null || str2 == null || str1.Length > str2.Length || str2.Length > str1.Length) return false;


            for (int i = 0; i < str1.Length; i++)
            {
                if (!str1[i].Equals(str2[i])) return false;
            }

            return true;
        }
        public bool compareArraySequence(string[] str1, string[] str2)
        {
            if (str1 == null || str2 == null || str1.Length > str2.Length || str2.Length > str1.Length) return false;

            for (int i = 0; i < str1.Length; i++)
            {
                if (str1[i] != str2[i]) return false;
            }

            return true;
        }

        public string byteToString(byte byte_buff)
        {
            string string_buff = "";
            string_buff += (char)(byte_buff);
            return string_buff;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////
///                                      Extensions                                         ///
///////////////////////////////////////////////////////////////////////////////////////////////

namespace Extensions
{
    public static class MyExtensions
    {
        public static byte[] byte_tab_buff;
        public static void WriteByte(this SerialPort sp, byte to_send)
        {
            byte_tab_buff = new byte[1];
            byte_tab_buff[0] = to_send;
            sp.Write(byte_tab_buff, 0, 1);
        }
    }
}
