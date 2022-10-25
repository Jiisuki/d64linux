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
using System.ComponentModel;


namespace D64IEC_USB_INTERFACE {
  /// <summary>
  /// Interaction logic for MainWindow.xaml
  /// </summary>
  public partial class MainWindow : Window, INotifyPropertyChanged {

    /* global disk system */
    d64 DISK;

    public int viewTrack;
    public int viewSector;
    public bool isBurning;

    /* global program files list for custom compilation */
    public List<prg> PRG;

    public MainWindow() {
      InitializeComponent();
      this.DataContext = this;

      DISK = new d64();
      PRG = new List<prg>();

      viewTrack = 0;
      viewSector = 0;
      isBurning = false;

      //DISK.Load( "minigame.d64" );

      /* Use only a 35-track disk at init point */
      UpdateBAM();
      UpdateDataView();
    }

    public void switchView( object sender, MouseButtonEventArgs e ) {

      Rectangle r = (Rectangle) e.Source;

      int x = (int) Canvas.GetLeft( r );
      int y = (int) Canvas.GetTop( r );

      viewTrack = y / 12;
      viewSector = x / 12;

      if( viewTrack >= DISK.DiskSize || viewTrack < 0 )
        viewTrack = 0;
      if( viewSector >= d64format.sectors[viewTrack] || viewSector < 0 )
        viewSector = 0;

      menuControl.SelectedIndex = 1;
      UpdateDataView();

      e.Handled = true;

    }

    public void colorizeSector( object sender, MouseEventArgs e ) {

      for( int i = 0; i < DiskSpaceGraph.Children.Count; i++ ) {
        Rectangle br = (Rectangle) DiskSpaceGraph.Children[i];
        br.StrokeThickness = 1;
      }

      Rectangle r = (Rectangle) e.Source;

      r.StrokeThickness = 2;

      e.Handled = true;
    }


    public void UpdateBAM() {
      DataUsage = 0;
      DiskSizeSect = 0;
      DataSpace0 = "";
      DataSpace1 = "";

      DiskSpaceGraph.Children.Clear();

      int boxSpacing = 2;

      for( int track = 0; track < DISK.DiskSize; track++ ) {
        DiskSizeSect += d64format.sectors[track];
        Boolean[] isFree = DISK.TrackSpaceFree( track + 1 );

        for( int sect = 0; sect < d64format.sectors[track]; sect++ ) {
          
          Rectangle r = new Rectangle();
          if( isFree[sect] ) {
            r.Fill = Brushes.Black;
          } else {
            r.Fill = Brushes.Firebrick;
            DataUsage++;
          }
          r.Stroke = Brushes.WhiteSmoke;
          r.SnapsToDevicePixels = true;
          r.StrokeThickness = 1;
          r.ToolTip = "Track/Sector: " + ( track + 1 ).ToString( "00" ) + "/" + sect.ToString( "00" );
          //r.Name = track.ToString() + "/" + sect.ToString();
          r.MouseLeftButtonDown += new MouseButtonEventHandler( switchView );
          r.MouseMove += new MouseEventHandler( colorizeSector );
          r.Width = 10;
          r.Height = 10;
          Canvas.SetLeft( r, (r.Width + boxSpacing) * sect );
          Canvas.SetTop( r, (r.Height + boxSpacing) * track );
          DiskSpaceGraph.Children.Add( r );
          
        }
      }

      DataUsageString = ( 100 - (100*DataUsage) / DiskSizeSect ).ToString() + " % free space";
      DiskName = DISK.diskName;

      DirectoryView = "";
      List<dirEntry> DIR = DISK.Directory;

      foreach( dirEntry d in DIR ){
        DirectoryView += d.Title + "   " + d.BlockSize.ToString( "000" ) + " BLOCKS   " + d.PRGExt + "\n";
      }

      viewTrack = 0;
      viewSector = 0;
      UpdateDataView();

    }

    public void UpdateDataView() {
      DataView0 = "";
      DataView1 = "";

      DataViewString = ( viewTrack + 1 ).ToString() + "/" + viewSector.ToString();
      diskSector Sector = DISK.ReadSector( viewTrack + 1, viewSector );
      
      for( int ix = 0; ix < d64format.SECTOR_SIZE; ix += 16 ) {
        var cnt = Math.Min( 16, d64format.SECTOR_SIZE - ix );
        var line = new Byte[cnt];
        Array.Copy( Sector.GetSectorData, ix, line, 0, cnt );
        DataView0 += BitConverter.ToString( line ) + "\n";
      }

      for( int ix = 0; ix < d64format.SECTOR_SIZE; ix += 32 ) {
        var cnt = Math.Min( 32, d64format.SECTOR_SIZE - ix );
        var line = new Byte[cnt];
        Array.Copy( Sector.GetSectorData, ix, line, 0, cnt );
        DataView1 += d64format.fromPETASCII( line ).ToUpper() + "\n";
      }
    }


    public void UpdateCompilationList() {
      CompilationList.Items.Clear();
      foreach( prg newPRG in PRG ) {
        CompilationList.Items.Add( newPRG.Name + "   " + Math.Ceiling( (double) newPRG.Size / 256 ).ToString() + " Sectors" );
      }
    }








    /* Bound variables */
    private int _DataUsage;
    public int DataUsage {
      get { return _DataUsage; }
      set { _DataUsage = value; RaisePropertyChanged( "DataUsage" ); }
    }

    private String _DataUsageString;
    public String DataUsageString {
      get { return _DataUsageString; }
      set { _DataUsageString = value; RaisePropertyChanged( "DataUsageString" ); }
    }

    private String _DiskName;
    public String DiskName {
      get { return _DiskName; }
      set { _DiskName = value; RaisePropertyChanged( "DiskName" ); }
    }

    private String _DataSpace0;
    public String DataSpace0 {
      get { return _DataSpace0; }
      set { _DataSpace0 = value; RaisePropertyChanged( "DataSpace0" ); }
    }

    private String _DataSpace1;
    public String DataSpace1 {
      get { return _DataSpace1; }
      set { _DataSpace1 = value; RaisePropertyChanged( "DataSpace1" ); }
    }

    private String _DirectoryView;
    public String DirectoryView {
      get { return _DirectoryView; }
      set { _DirectoryView = value; RaisePropertyChanged( "DirectoryView" ); }
    }

    private String _DataView0;
    public String DataView0 {
      get { return _DataView0; }
      set { _DataView0 = value; RaisePropertyChanged( "DataView0" ); }
    }

    private String _DataView1;
    public String DataView1 {
      get { return _DataView1; }
      set { _DataView1 = value; RaisePropertyChanged( "DataView1" ); }
    }

    private String _DataViewString;
    public String DataViewString {
      get { return _DataViewString; }
      set { _DataViewString = value; RaisePropertyChanged( "DataViewString" ); }
    }

    private String _CustomFileName;
    public String CustomFileName {
      get { return _CustomFileName; }
      set { _CustomFileName = value; RaisePropertyChanged( "CustomFileName" ); }
    }

    private int _DiskSizeSect;
    public int DiskSizeSect {
      get { return _DiskSizeSect; }
      set { _DiskSizeSect = value; RaisePropertyChanged( "DiskSizeSect" ); }
    }

    private int _BurnSector;
    public int BurnSector {
      get { return _BurnSector; }
      set { _BurnSector = value; RaisePropertyChanged( "BurnSector" ); }
    }

    /* INotifyProperty stuff */
    public event PropertyChangedEventHandler PropertyChanged;

    public void RaisePropertyChanged( String propName ) {
      if( PropertyChanged != null )
        PropertyChanged( this, new PropertyChangedEventArgs( propName ) );
    }




    /* button callbacks */
    private void btnLoadDisk( object sender, RoutedEventArgs e ) {
      Microsoft.Win32.OpenFileDialog dlg = new Microsoft.Win32.OpenFileDialog();
      dlg.DefaultExt = ".d64";
      dlg.Filter = "C64 Disk Images (*.d64)|*.d64";

      Nullable<Boolean> result = dlg.ShowDialog();

      if( result == true ) {
        DISK.Load( dlg.FileName );
        UpdateBAM();
      }
    }


    private void btnTrackPrev( object sender, RoutedEventArgs e ) {
      if( --viewTrack < 0 ) {
        viewTrack = DISK.DiskSize - 1;
        viewSector = 0;
      }
      UpdateDataView();
    }

    private void btnTrackNext( object sender, RoutedEventArgs e ) {
      if( ++viewTrack >= DISK.DiskSize ) {
        viewTrack = 0;
        viewSector = 0;
      }
      UpdateDataView();
    }

    private void btnSectorPrev( object sender, RoutedEventArgs e ) {
      if( --viewSector < 0 ) {
        if( --viewTrack < 0 )
          viewTrack = DISK.DiskSize - 1;
        viewSector = d64format.sectors[viewTrack] - 1;
      }
      UpdateDataView();
    }

    private void btnSectorNext( object sender, RoutedEventArgs e ) {
      if( ++viewSector >= d64format.sectors[viewTrack] ) {
        viewSector = 0;
        if( ++viewTrack >= DISK.DiskSize )
          viewTrack = 0;
      }
      UpdateDataView();
    }

    private void btnBurnDisk( object sender, RoutedEventArgs e ) {

      if( isBurning )
        return;

      BurnSector = 0;

      String ComPort = BurnerPort.Text;
      String mOK = SerialCom.SetupCOM( ComPort );
      if( mOK.Length > 0 ) {
        MessageBox.Show( mOK, "Communication Trouble", MessageBoxButton.OK, MessageBoxImage.Asterisk );
        return;
      }
      
      if( !SerialCom.ComActive )
        return;

      isBurning = true;

      new Thread( () => {
        Thread.CurrentThread.IsBackground = true;

        DateTime a = DateTime.UtcNow;

        /* erase disk, wait for ack */
        SerialCom.SendByte( 0xed );

        if( !AckRecieved() ) { isBurning = false; goto W0_ABORT; }

        MessageBox.Show( "Format completed!" );

        
        List<diskTrack> DiskImage = DISK.DiskImage;
        for( int ti = 0; ti < DiskImage.Count; ti++ ){
          for( int si = 0; si < DiskImage[ti].Sector.Count; si++ ){
            Byte[] ts = { (Byte) ti, (Byte) si };
            SerialCom.SendByte( 0xbd ); // opcode bd = burn disk
            SerialCom.SendArray( ts ); // address, track + sector

            if( !AckRecieved() ) { isBurning = false; goto W0_ABORT; }

            SerialCom.SendArray( DiskImage[ti].Sector[si].GetSectorData ); // complete sector bytes

            /* now we need to wait for programming acknowledge! */
            if( !AckRecieved() ) { isBurning = false; goto W0_ABORT; }

            BurnSector++;
          }
        }
W0_ABORT:
        if( !isBurning ) {
          /* something went wrong here... */
          MessageBox.Show( "Error during programming. Check connections.", "Program Error", MessageBoxButton.OK, MessageBoxImage.Error );
          SerialCom.Close();
        } else {
          MessageBox.Show( "Burned " + BurnSector.ToString() +
                          " out of " + DiskSizeSect.ToString() +
                          " sectors.\n\nTime to burn: " +
                          ( DateTime.UtcNow - a ).TotalSeconds.ToString() + " seconds." );
          isBurning = false;
          SerialCom.Close();
        }
      } ).Start();

    }

    private void btnQuickBurn( object sender, RoutedEventArgs e ) {

      if( isBurning )
        return;

      BurnSector = 0;

      if( !SerialCom.ComActive ) {
        String ComPort = BurnerPort.Text;
        String mOK = SerialCom.SetupCOM( ComPort );
        if( mOK.Length > 0 ) {
          MessageBox.Show( mOK, "Communication Trouble", MessageBoxButton.OK, MessageBoxImage.Asterisk );
          return;
        }
      }

      if( !SerialCom.ComActive )
        return;

      isBurning = true;

      new Thread( () => {
        Thread.CurrentThread.IsBackground = true;
        int onlyBurned = 0;

        DateTime a = DateTime.UtcNow;

        /* erase disk, wait for ack */
        SerialCom.SendByte(0xed);
        if( !AckRecieved() ) { isBurning = false; goto W1_ABORT; }

        List<diskTrack> DiskImage = DISK.DiskImage;
        for( int ti = 0; ti < DiskImage.Count; ti++ ) {
          for( int si = 0; si < DiskImage[ti].Sector.Count; si++ ) {
            if( DiskImage[ti].Sector[si].Free == false ) {
              Byte[] ts = { (Byte) ti, (Byte) si };
              SerialCom.SendByte( 0xbd ); // opcode bd = burn disk
              SerialCom.SendArray( ts ); // we needed this address-tuple to enable quick burn.

              /* here we wait for an ack, so we know we are ready to blast data */
              if( !AckRecieved() ) { isBurning = false; goto W1_ABORT; }
              
              SerialCom.SendArray( DiskImage[ti].Sector[si].GetSectorData );

              /* now we need to wait for programming acknowledge! */
              if( !AckRecieved() ) { isBurning = false; goto W1_ABORT; }

              onlyBurned++;
            }
            BurnSector++;
          }
        }
W1_ABORT:
        if( !isBurning ) {
          /* something went wrong here... */
          MessageBox.Show( "Error during programming. Check connections.", "Program Error", MessageBoxButton.OK, MessageBoxImage.Error );
          SerialCom.Close();
        } else {
          BurnSector = DiskSizeSect;
          MessageBox.Show( "Burned " + onlyBurned.ToString() +
                           " out of " + DiskSizeSect.ToString() +
                           " sectors (" + ( 100 * onlyBurned / DiskSizeSect ).ToString() +
                           " %).\n\nTime to burn: " + ( DateTime.UtcNow - a ).TotalSeconds.ToString() + " seconds." );
          isBurning = false;
          SerialCom.Close();
        }
      } ).Start();

    }

    private void Window_MouseMove( object sender, MouseEventArgs e ) {
      this.DragMove();
    }

    private void btnQuit( object sender, RoutedEventArgs e ) {
      SerialCom.Close();
      this.Close();
    }

    private void btnAddFileToCompilation( object sender, RoutedEventArgs e ) {
      Microsoft.Win32.OpenFileDialog dlg = new Microsoft.Win32.OpenFileDialog();
      dlg.DefaultExt = ".prg";
      dlg.Filter = "C64 Program (*.PRG)|*.PRG";
      dlg.Multiselect = true;

      Nullable<Boolean> result = dlg.ShowDialog();

      if( result == true ) {
        foreach( String FileName in dlg.FileNames ) {
          prg newPRG = new prg( FileName );
          PRG.Add( newPRG );
          UpdateCompilationList();
          DataUsage += (int) Math.Ceiling( (double) newPRG.Size / 256 );
          DataUsageString = ( 100 - ( 100 * DataUsage ) / DiskSizeSect ).ToString() + " % free space";
        }
      }
    }

    private void btnCreateNewCompilation( object sender, RoutedEventArgs e ) {
      DISK.Format( d64format.diskSizeType.STANDARD );
      UpdateBAM();
      UpdateCompilationList();
    }

    private void btnRemoveFileFromCompilation( object sender, RoutedEventArgs e ) {
      if( CompilationList.Items.Count < 1 )
        return;
      CompilationList.Items.RemoveAt( CompilationList.SelectedIndex );
      UpdateCompilationList();
    }

    private void btnGenerateDisk( object sender, RoutedEventArgs e ) {
      DISK.GenerateDisk( PRG, DiskName );
      UpdateBAM();
      UpdateDataView();
    }

    private void btnSaveCompilation( object sender, RoutedEventArgs e ) {
      Microsoft.Win32.SaveFileDialog dlg = new Microsoft.Win32.SaveFileDialog();
      dlg.DefaultExt = ".d64";
      dlg.Filter = "C64 Diskette (*.d64)|*.d64";

      Nullable<Boolean> result = dlg.ShowDialog();

      if( result == true ) {
        DISK.SaveDiskette( dlg.FileName );
      }
    }

    private void btnMemoryView( object sender, RoutedEventArgs e ) {
      menuControl.SelectedIndex = 0;
    }

    private void btnUpdateCustomFileName( object sender, RoutedEventArgs e ) {
      PRG[CompilationItemSelection].Name = CustomFileName;
      UpdateCompilationList();
    }

    public int CompilationItemSelection = 0;
    private void CompilationList_SelectionChanged( object sender, SelectionChangedEventArgs e ) {
      if( CompilationList.SelectedIndex >= 0 )
        CompilationItemSelection = CompilationList.SelectedIndex;
      CustomFileName = PRG[CompilationItemSelection].Name;
    }











    private void btnDownloadVerify( object sender, RoutedEventArgs e ) {
      if( isBurning )
        return;

      BurnSector = 0;

      if( !SerialCom.ComActive ) {
        String ComPort = BurnerPort.Text;
        String mOK = SerialCom.SetupCOM( ComPort );
        if( mOK.Length > 0 ) {
          MessageBox.Show( mOK, "Communication Trouble", MessageBoxButton.OK, MessageBoxImage.Asterisk );
          return;
        }
      }

      if( !SerialCom.ComActive )
        return;

      isBurning = true; /* rename flag? */
      
      /* create an empty disk for storing read data */
      d64 verifyDisk = new d64();
      verifyDisk.Format( d64format.diskSizeType.STANDARD );

      new Thread( () => {
        Thread.CurrentThread.IsBackground = true;

        /* first, send read reset */
        SerialCom.SendByte( 0xfa );

        DateTime a = DateTime.UtcNow; /* start timer */
        
        int ti = 0;
        int si = 0;
        Byte bi = 0;

        int tmp = waitForByte();
        if( tmp > 255 ) {
          /* timeout */
          SerialCom.Close();
          MessageBox.Show( "Timeout" );
          return;
        }

        /* write first byte and increment counter */
        verifyDisk.WriteDiskByte( 1, 0, 0, (Byte) tmp );
        bi++;

        /* now we use sequential read to get all data */
        while( true ) {

          SerialCom.SendByte( 0xfd ); /* normal progressive read */

          tmp = waitForByte();
          if( tmp > 255 ) {
            /* we got a timeout */
            isBurning = false;
            break;
          }

          verifyDisk.WriteDiskByte( ti + 1, si, bi, (Byte) tmp );

          if( ++bi == 0 ) {
            /* byte spilled over */
            //break; /* temp */
            if( ++si >= verifyDisk.DiskImage[ti].Sector.Count ) {
              si = 0;
              if( ++ti >= verifyDisk.DiskImage.Count ) {
                break;
              }
            }
            BurnSector++;
          }

        }
        BurnSector = DiskSizeSect;

        if( !isBurning ) {
          MessageBox.Show( "Read timed out!", "Verify Error", MessageBoxButton.OK, MessageBoxImage.Error );
        } else {
          MessageBox.Show( "Read disk in " + ( DateTime.UtcNow - a ).TotalSeconds.ToString() + " seconds." );
          isBurning = false;
          DISK = new d64(verifyDisk.DiskImage);
        }
        SerialCom.Close();

      } ).Start();

      UpdateBAM();
      UpdateDataView();

    }





















    private void btnReadFlashStatusRegister(object sender, RoutedEventArgs e) {
      
      String ComPort = BurnerPort.Text;
      String mOK = SerialCom.SetupCOM( ComPort );
      if( mOK.Length > 0 ) {
        MessageBox.Show( mOK, "Communication Trouble", MessageBoxButton.OK, MessageBoxImage.Asterisk );
        return;
      }

      if( !SerialCom.ComActive )
        return;

      /* send op to read flash status register */
      SerialCom.SendByte( 0x68 );

      int tmp = waitForByte();
      if( tmp > 255 )
        return;

      Byte stat = (Byte) tmp;

      uint[] sbin = new uint[8];
      for( int i = 0; i < 8; i++ ) {
        sbin[7 - i] = (uint) (stat >> i) & 0x01;  
      }

      String msg = "Status Register = " + stat.ToString() + "\n\n" + "binary: ";
      foreach( uint x in sbin ) {
        msg += x.ToString() + " ";
      }

      MessageBox.Show( msg );
      SerialCom.Close();
      
    }




    private void btnWriteTestByte(object sender, RoutedEventArgs e) {

      String ComPort = BurnerPort.Text;
      String mOK = SerialCom.SetupCOM( ComPort );
      if( mOK.Length > 0 ) {
        MessageBox.Show( mOK, "Communication Trouble", MessageBoxButton.OK, MessageBoxImage.Asterisk );
        return;
      }

      if( !SerialCom.ComActive )
        return;

      /* send burn byte (store 0xbb) */
      Byte[] cmd = { 0xbb };
      SerialCom.SendArray( cmd );

      if( !AckRecieved() )
        return;

      MessageBox.Show( "Done!" );
      SerialCom.Close();

    }

    private void btnReadTestByte(object sender, RoutedEventArgs e) {

      String ComPort = BurnerPort.Text;
      String mOK = SerialCom.SetupCOM( ComPort );
      if( mOK.Length > 0 ) {
        MessageBox.Show( mOK, "Communication Trouble", MessageBoxButton.OK, MessageBoxImage.Asterisk );
        return;
      }

      if( !SerialCom.ComActive )
        return;

      /* read test byte */
      SerialCom.SendByte(0xfb);

      int tmp = waitForByte();
      if( tmp > 255 )
        return;
      
      /* seems like we have data, lets display it! */
      Byte stat = (Byte) tmp;

      uint[] sbin = new uint[8];
      for( int i = 0; i < 8; i++ ) {
        sbin[7 - i] = (uint) (stat >> i) & 0x01;
      }

      String msg = "Test Byte = " + stat.ToString() + "\n\n" + "binary: ";
      foreach( uint x in sbin ) {
        msg += x.ToString() + " ";
      }

      MessageBox.Show( msg );
      SerialCom.Close();

    }

    private void btnIdentifyDisk(object sender, RoutedEventArgs e) {
      Byte op = 0x48;
    }




    private bool AckRecieved(){
      DateTime a = DateTime.UtcNow;
      DateTime ws = DateTime.UtcNow;
      while( !SerialCom.DataAvailable ) {
        if( (DateTime.UtcNow - ws).TotalSeconds > 3 ) {
          /* 3 second timeout */
          SerialCom.Close();
          MessageBox.Show( "Timeout!" );
          return false;
        }
      }

      /* seems like we have data, lets display it! */
      if( SerialCom.ReadByte() != 0xf0 ) {
        /* got something else than an ack! */
        MessageBox.Show( "Not an acknowledge!" );
        SerialCom.Close();
        return false;
      }

      return true;

    }

    private int waitForByte(){
      DateTime a = DateTime.UtcNow;
      DateTime ws = DateTime.UtcNow;
      while( !SerialCom.DataAvailable ) {
        if( (DateTime.UtcNow - ws).TotalSeconds > 3 ) {
          /* 3 second timeout */
          SerialCom.Close();
          MessageBox.Show( "Timeout!" );
          return 256;
        }
      }

      return (int) SerialCom.ReadByte();
    }




  }
}
