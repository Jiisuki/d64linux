using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;
using System.IO.Ports;
using System.Windows;

namespace D64IEC_USB_INTERFACE {

  public static class d64format {
    public static int[] sectors = {21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
                                   21, 21, 21, 21, 21, 21, 21, 19, 19, 19,
                                   19, 19, 19, 19, 18, 18, 18, 18, 18, 18,
                                   17, 17, 17, 17, 17, 17, 17, 17, 17, 17};
    public static int[] offsets = {     0,   5376,  10752,  16128,  21504,  26880,  32256,  37632,  43008,  48384,
                                    53760,  59136,  64512,  69888,  75264,  80640,  86016,  91392,  96256, 101120,
                                   105984, 110848, 115712, 120576, 125440, 130048, 134656, 139264, 143872, 148480,
                                   153088, 157440, 161792, 166144, 170496, 174848, 179200, 183552, 187904, 192256};

    public static int DIR_TRACK = 18;
    public static int BAM_TRACK = 18;
    public static int SECTOR_SIZE = 256;
    public static int DIR_ENTRY_SIZE = 32;
    public static int NAME_LENGTH = 16;
    public static int BLOCK_SIZE = 254;

    public enum diskSizeType{
      STANDARD = 35,
      EXT1 = 36,
      EXT2 = 37,
      EXT3 = 38,
      EXT4 = 39,
      EXT5 = 40
    }


    public static String fromPETASCII( Byte[] binaryData ) {
      for( int i = 0; i < binaryData.Length; i++ ) {
        if( binaryData[i] < 32 || ( binaryData[i] >= 127 && binaryData[i] < 193 ) || binaryData[i] >= 219 ) {
          binaryData[i] = 32;
        } else if( binaryData[i] >= 193 && binaryData[i] < 219 ) {
          binaryData[i] -= 161;
        }
      }
      return Encoding.ASCII.GetString( binaryData );
    }



  }




  public class diskSector{
    private Byte[] _Data;
    public diskSector() {
      _Data = new Byte[ d64format.SECTOR_SIZE ];
    }
    public Boolean Free{
      get {
        Boolean isFree = true;
        foreach( Byte iByte in _Data ) {
          if( iByte != 0 )
            isFree = false;
        }
        return isFree;
      }
    }
    public Byte[] GetSectorData {
      get { return _Data; }
    }
    public void SetSectorByte( Byte index, Byte value ) {
      _Data[index] = value;
    }
    public Byte GetSectorByte( Byte index ) {
      return _Data[index];
    }
    public Byte GetSectorByte( int index ) {
      return _Data[index];
    }
    public Byte[] GetSectorByte( Byte index, Byte count ) {
      var n = new Byte[count];
      Array.Copy( _Data, index, n, 0, count );
      return n;
    }
    public Byte[] GetSectorByte( int index, int count ) {
      var n = new Byte[count];
      Array.Copy( _Data, index, n, 0, count );
      return n;
    }
  }




  public class diskTrack {
    public List<diskSector> Sector;
    public int Offset;
    public diskTrack(int TrackNumber) {
      // TrackNumber starts on 1
      Sector = new List<diskSector>();
      for( int i = 0; i < d64format.sectors[TrackNumber - 1]; i++ ) {
        Sector.Add( new diskSector() );
      }
      Offset = d64format.offsets[TrackNumber - 1];
    }
  }






  public class dirEntry {
    private String _title;
    private Byte _nextDirTrack;
    private Byte _nextDirSector;
    private String _prgExtension;
    private int _blockSize;
    private Byte _onTrack;
    private Byte _onSector;
    public dirEntry() {
      _title = "";
      _nextDirTrack = 0;
      _nextDirSector = 0;
      _prgExtension = "";
      _blockSize = 0;
      _onTrack = 0;
      _onSector = 0;
    }
    public void SetTitle( Byte[] titleBytes ) {
      _title = d64format.fromPETASCII( titleBytes );
    }
    public String Title {
      get { return _title; }
      set { _title = value; }
    }
    public String PRGExt {
      get { return _prgExtension; }
      set { _prgExtension = value; }
    }
    public Byte NextDirTrack {
      get { return _nextDirTrack; }
      set { _nextDirTrack = value; }
    }
    public Byte NextDirSector {
      get { return _nextDirSector; }
      set { _nextDirSector = value; }
    }
    public Byte FirstTrack {
      get { return _onTrack; }
      set { _onTrack = value; }
    }
    public Byte FirstSector {
      get { return _onSector; }
      set { _onSector = value; }
    }
    public void SetSize( Byte[] blBytes ) {
      _blockSize = blBytes[0] + blBytes[1] * d64format.SECTOR_SIZE;
    }
    public int BlockSize {
      get { return _blockSize; }
    }
  }








  /* Serial com */
  public static class SerialCom {
    private static SerialPort Serial = new SerialPort();
    public static Boolean ComActive = false;

    public static String SetupCOM( String Port ) {
      try {
        //Serial.BaudRate = 115200;
        //Serial.BaudRate = 9600;
        Serial.BaudRate = 19200;
        //Serial.BaudRate = 57600;
        Serial.DataBits = 8;
        Serial.StopBits = StopBits.One;
        Serial.Parity = Parity.None;
        Serial.PortName = Port;

        Serial.WriteBufferSize = d64format.SECTOR_SIZE;

        Serial.Open();
      } catch( Exception ex ) {
        Serial.Close();
        ComActive = false;
        return ex.Message;
      }
      ComActive = Serial.IsOpen;
      return "";
    }

    public static void SendArray( Byte[] bArray ) {
      if( !Serial.IsOpen )
        return;
      Serial.Write( bArray, 0, bArray.Length );
    }

    public static void SendByte( Byte bByte ) {
      if( !Serial.IsOpen )
        return;
      Byte[] tByte = {bByte};
      Serial.Write( tByte, 0, 1 );
    }

    public static Boolean DataAvailable {
      get { if( Serial.BytesToRead > 0 ) return true; return false; }
    }

    public static Byte ReadByte() {
      int tb = Serial.ReadByte();
      return (Byte) tb;
    }

    public static void Close() {
      Serial.Close();
    }

    public static int BytesAvailable {
      get { return Serial.BytesToRead; }
    }
  }




  public class prg {
    private Byte[] _data;
    private String _filename;
    private int _size;
    private String _name;

    public prg() {
      _name = "";
      _filename = "";
      _size = 0;
      _data = new Byte[0];
    }
    public prg( String Filename ) {
      if( Filename.Length > 16 ){
        _name = Filename.Substring( Filename.Length - 20, 16 );
      } else {
        _name = "      ----      ";
      }
      _filename = Filename;
      Load();
    }

    private void Load() {

      var fs = new FileStream( _filename, FileMode.Open );
      _size = (int) fs.Length;
      _data = new Byte[_size];

      int rPos = 0;
      while( rPos < _size ) {
        _data[rPos] = (Byte) fs.ReadByte();
        rPos++;
      }
      fs.Close();
    }

    public Byte[] Data {
      get { return _data; }
    }
    public int Size {
      get { return _size; }
    }
    public String Filename {
      get { return _filename; }
    }
    public String Name {
      get {
        if( _name.Length < d64format.NAME_LENGTH )
          return _name.PadRight( d64format.NAME_LENGTH );
        return _name; }
      set {
        if( value.Length > 16 ) {
          _name = value.Substring( 0, 16 );
        } else {
          _name = value;
        }
      }
    }
  }



  public class d64 {

    private List<diskTrack> _diskImage;
    private String _diskName;
    private Byte _diskDOS;
    private Byte[] _diskID;
    private Byte[] _diskBAM;
    private List<dirEntry> _directory;

    public d64() {
      /* init */
      Format( d64format.diskSizeType.STANDARD );
    }

    public d64(List<diskTrack> newImage ){
      Format( d64format.diskSizeType.STANDARD );
      _diskImage = newImage;
      readBAM();
      readDIR();
    }

    public void Load( String filename ) {
      Format(d64format.diskSizeType.STANDARD);

      var fs = new FileStream( filename, FileMode.Open );
      var len = (int) fs.Length;

      int curTrack = 0;
      int curSector = 0;
      Byte curByte = 0;
      int rPos = 0;
      while( rPos < len ) {
        _diskImage[curTrack].Sector[curSector].SetSectorByte( curByte, (Byte) fs.ReadByte() );
        if( ++curByte == 0 ) {
          // we spilled over
          if( ++curSector >= d64format.sectors[curTrack] ) {
            // we filled the track
            curSector = 0;
            curTrack++;
          }
        }
        rPos++;
      }

      fs.Close();

      readBAM();
      readDIR();

    }
    public void Format( d64format.diskSizeType diskSize ) {
      _diskImage = new List<diskTrack>();
      for( int i = 0; i < (int) diskSize; i++ ) {
        _diskImage.Add( new diskTrack( i + 1 ) );
      }
      _diskName = "";
      _diskDOS = 0x41;
      _diskID = new Byte[2];
      _diskID[0] = 0x00; _diskID[1] = 0x00;
      _diskBAM = new Byte[0x8c];
      for( int k = 0; k < 0x8c; k++ )
        _diskBAM[k] = 0xff;

      _directory = new List<dirEntry>();
    }

    public Boolean[] TrackSpaceFree( int Track ) {
      Boolean[] isFree = new Boolean[d64format.sectors[Track - 1]];
      for( int i = 0; i < isFree.Length; i++ ) {
        isFree[i] = _diskImage[Track - 1].Sector[i].Free;
      }
      return isFree;
    }

    public String diskName {
      get { return _diskName; }
    }
    public int NumberOfEntries {
      get { return _directory.Count; }
    }
    public List<dirEntry> Directory {
      get { return _directory; }
    }
    public int DiskSize {
      get { return _diskImage.Count; }
    }
    public diskSector ReadSector( Byte Track, Byte Sector ) {
      return _diskImage[Track - 1].Sector[Sector];
    }
    public diskSector ReadSector( int Track, int Sector ) {
      return _diskImage[Track - 1].Sector[Sector];
    }
    public List<diskTrack> DiskImage {
      get { return _diskImage; }
    }

    public void WriteDiskByte( int Track, int Sector, Byte ByteIndex, Byte Data ) {
      _diskImage[Track - 1].Sector[Sector].SetSectorByte( ByteIndex, Data );
    }

    /* class internal stuff */
    private void readBAM() {
      // filename for disk in offset+144 to offset+159
      var n = new Byte[d64format.NAME_LENGTH];
      Array.Copy( _diskImage[d64format.BAM_TRACK - 1].Sector[0].GetSectorData, 0x90, n, 0, d64format.NAME_LENGTH );
      _diskName = d64format.fromPETASCII( n );
      // disk dos version
      _diskDOS = _diskImage[d64format.BAM_TRACK - 1].Sector[0].GetSectorByte( 0x02 );
      // disk id
      Array.Copy( _diskImage[d64format.BAM_TRACK - 1].Sector[0].GetSectorData, 0xa2, _diskID, 0, 2 );
      // bam data
      Array.Copy( _diskImage[d64format.BAM_TRACK - 1].Sector[0].GetSectorData, 0x04, _diskBAM, 0, 0x8c );
    }

    private void readDIR() {
      
      int cSector = 1;
      int cTrack = d64format.DIR_TRACK - 1;
      int nextTrack = 0;
      int nextSector = 0;
      Byte entryFT = 0;

      while( true ) {
        for( int k = 0; k < 8; k++ ) {
          int offset = k * 32;
          dirEntry newEntry = new dirEntry();
          newEntry.NextDirTrack = _diskImage[cTrack].Sector[cSector].GetSectorByte( offset );
          newEntry.NextDirSector = _diskImage[cTrack].Sector[cSector].GetSectorByte( offset + 1 );
          if( k == 0 ) {
            nextTrack = newEntry.NextDirTrack;
            nextSector = newEntry.NextDirSector;
          }
          entryFT = _diskImage[cTrack].Sector[cSector].GetSectorByte( offset + 2 );
          newEntry.PRGExt = getFileType( entryFT );
          newEntry.FirstTrack = _diskImage[cTrack].Sector[cSector].GetSectorByte( offset + 3 );
          newEntry.FirstSector = _diskImage[cTrack].Sector[cSector].GetSectorByte( offset + 4 );
          newEntry.SetTitle( _diskImage[cTrack].Sector[cSector].GetSectorByte( offset + 5, d64format.NAME_LENGTH ) );
          newEntry.SetSize( _diskImage[cTrack].Sector[cSector].GetSectorByte( offset + 30, 2 ) );

          if( entryFT != 0 )
            _directory.Add(newEntry);

        }
        if( nextTrack == 0 ) {
          return;
        } else {
          cTrack = nextTrack - 1;
          cSector = nextSector;
        }
      }
    }

    private String getFileType( Byte fileType ) {
      switch( fileType & 7 ) {
        case 0:
          return "DEL";
        case 1:
          return "SEQ";
        case 2:
          return "PRG";
        case 3:
          return "USR";
        case 4:
          return "REL";
        default:
          return "*";
      }
    }

    public void GenerateDisk( List<prg> Programs, String DiskName ) {

      Format( d64format.diskSizeType.STANDARD );
      _diskName = DiskName;

      int t = 0;
      int s = 0;
      int nt = 0;
      int ns = 0;
      foreach( prg PRG in Programs ) {

        dirEntry newEntry = new dirEntry();

        t = 0;
        s = 0;
        nt = 0;
        ns = 0;
        // check for next free track/sector
        while( _diskImage[t].Sector[s].Free == false ) {
          if( ++s >= d64format.sectors[t] ) {
            s = 0;
            if( ++t >= _diskImage.Count )
              return; // full disk
            if( t == 17 ) // dir
              t++;
          }
        }
        nt = t;
        ns = s;

        newEntry.FirstTrack = (Byte) t;
        newEntry.FirstSector = (Byte) s;

        _directory.Add(newEntry);
        
        Byte[] pData = PRG.Data;
        int b = 0;
        while( b < PRG.Size ) {
          for( Byte k = 0; k < d64format.SECTOR_SIZE; k++ ) {
            if( k == 0 ){
              if( ++ns >= d64format.sectors[t] ) {
                if( ++nt == 17 ) // directory!
                  nt++;
                ns = 0;
              }
              _diskImage[t].Sector[s].SetSectorByte( k, (Byte) (nt+1) );
            } else if( k == 1 ) {
              _diskImage[t].Sector[s].SetSectorByte( k, (Byte) ns );
            } else {
              _diskImage[t].Sector[s].SetSectorByte( k, pData[b] );
              if( ++b >= PRG.Size ) {
                // we reached the end before sector is finished
                _diskImage[t].Sector[s].SetSectorByte( 0, (Byte) 0 );
                _diskImage[t].Sector[s].SetSectorByte( 1, (Byte) 0 );
                break;
              }
            }
            if( k == d64format.SECTOR_SIZE - 1 ) {
              s = ns;
              t = nt;
            }
          }
          
        }
      }

      /* write directory */

      t = 17;
      s = 1;
      ns = s;
      nt = t;

      Byte offset = 0;

      foreach( prg PRG in Programs ) {
        if( offset == 0 ) {
          if( ++ns >= d64format.sectors[t] ) {
            ns = 0;
            nt++;
          }
          if( Programs.IndexOf( PRG ) == Programs.Count-1 ) {
            _diskImage[t].Sector[s].SetSectorByte( offset, 0 );
            _diskImage[t].Sector[s].SetSectorByte( (Byte) (offset + 1), 0 );
          } else {
            _diskImage[t].Sector[s].SetSectorByte( offset, (Byte) nt );
            _diskImage[t].Sector[s].SetSectorByte( (Byte) (offset + 1), (Byte) ns );
          }
        } else {
          _diskImage[t].Sector[s].SetSectorByte( offset, 0 );
          _diskImage[t].Sector[s].SetSectorByte( (Byte) (offset + 1), 0 );
        }
        
        _diskImage[t].Sector[s].SetSectorByte( (Byte) (offset + 2), 0x82 );
        _diskImage[t].Sector[s].SetSectorByte( (Byte) (offset + 3), _directory[Programs.IndexOf( PRG )].FirstTrack );
        _diskImage[t].Sector[s].SetSectorByte( (Byte) (offset + 4), _directory[Programs.IndexOf( PRG )].FirstSector );
        for( Byte k = 0; k < d64format.NAME_LENGTH; k++ ) {
          _diskImage[t].Sector[s].SetSectorByte( (Byte) (offset+5+k), (Byte) PRG.Name[k] );
        }
        _diskImage[t].Sector[s].SetSectorByte( (Byte) (offset + 0x1e), (Byte) ( (PRG.Size/256) & 0xff ) );
        _diskImage[t].Sector[s].SetSectorByte( (Byte) (offset + 0x1f), (Byte) ( (PRG.Size/256) >> 8 ) );

        offset += 32;
        if( offset > 255 ) {
          offset = 0;
          t = nt;
          s = ns;
        }

      }

      _directory.Clear();
      readBAM();
      readDIR();

    }

    public void SaveDiskette( String Filename ) {
      var fs = new FileStream( Filename, FileMode.Create );
      
      for( int t = 0; t < _diskImage.Count; t++ ){
        for( int s = 0; s < _diskImage[t].Sector.Count; s++ ) {
          Byte[] dData = _diskImage[t].Sector[s].GetSectorData;
          foreach( Byte b in dData ) {
            fs.WriteByte( b );
          }
        }
      }
      fs.Close();
    }

    

  }

}
