var tinyinflate = require('./tiny-inflate');
var path = require('./path');

const regeneratorRuntime = require('./regenerator-runtime');
const co = require('./co');

function ZipData() {
    this.files = {};
    this.directories = {};
    this.record = null;
  }

  function FileBase() {}

  FileBase.prototype = {
    version : 0,
    bitFlag : 0,
    method : 0,
    fileTime : 0,
    fileDate : 0,
    crc32 : 0,
    size : 0,
    fileSize : 0,
    nameLength : 0,
    extraLength : 0,
    name : null,
    extra : null,
    modified : function () {
      return new Date(
        1980 + (this.fileDate >> 9),
        (this.fileDate >> 5 & 15) - 1,
        this.fileDate & 31,
        this.fileTime >> 11,
        this.fileTime >> 5 & 60
      );
    }
  };

  function File(b) {
    this.version = b.read_int16();
    this.bitFlag = b.read_int16();
    this.method = b.read_int16();
    this.fileTime = b.read_int16();
    this.fileDate = b.read_int16();
    this.crc32 = b.read_int();
    this.size = b.read_int();
    this.fileSize = b.read_int();
    this.nameLength = b.read_int16();
    this.extraLength = b.read_int16();
    this.name = path.normalize(b.read_text(this.nameLength));
    this.extra = b.read_ascii(this.extraLength);
    this.data = b.read(this.size);
  }

  File.prototype = new FileBase;
  File.prototype.data = null;
  File.prototype.header = null;
  File.prototype.inflate = function () {
    var blob = this.data;
    if ( this.method == 0 ) {
      return blob;
    } else {
      return JSInflate.inflate(blob, this.fileSize);
    }
  };

  File.Header = function (b) {
    this.crc32 = b.read_int();
    this.size = b.read_int();
    this.fileSize = b.read_int();
  };

  File.Header.prototype = {
    crc32 : 0,
    size : 0,
    fileSize : 0
  };

  function Directory(b) {
    this.version = b.read_int16();
    this.extVersion = b.read_int16();
    this.bitFlag = b.read_int16();
    this.method = b.read_int16();
    this.fileTime = b.read_int16();
    this.fileDate = b.read_int16();
    this.crc32 = b.read_int();
    this.size = b.read_int();
    this.fileSize = b.read_int();
    this.nameLength = b.read_int16();
    this.extraLength = b.read_int16();
    this.commentLength = b.read_int16();
    this.diskNumberStart = b.read_int16();
    this.attributes = b.read_int16();
    this.extAttributes = b.read_int();
    this.headerOffset = b.read_int();
    this.name = b.read_text(this.nameLength);
    this.extra = b.read_text(this.extraLength);
    this.comment = b.read_text(this.commentLength);
  }

  Directory.prototype = new FileBase;
  Directory.prototype.extVersion = 0;
  Directory.prototype.commentLength = null;
  Directory.prototype.comment = null;
  Directory.prototype.diskNumberStart = 0;
  Directory.prototype.attributes = 0;
  Directory.prototype.extAttributes = 0;
  Directory.prototype.headerOffset = 0;
  Directory.prototype.record = null;

  Directory.Record = function (b) {
    this.diskNumber = b.read_int16();
    this.startNumber = b.read_int16();
    this.diskLength = b.read_int16();
    this.length = b.read_int16();
    this.directorySize = b.read_int();
    this.offset = b.read_int();
    this.commentLength = b.read_int16();
    this.comment = b.read_text(this.commentLength);
  };

  Directory.Record.prototype = {
    diskNumber : 0,
    startNumber : 0,
    diskLength : 0,
    length : 0,
    directorySize : 0,
    offset : 0,
    commentLength : 0,
    comment : null
  };

  function ByteReader(bytes) {
    this.bytes = new Uint8Array( bytes );
    this.length = this.bytes.length;
  }

  ByteReader.prototype = {
    position : 0,
    eos : function () {
      return this.position >= this.length;
    },
      slice : function (start, end) {
          var b = this.bytes.subarray(start, end);
          return b;
      },
    read : function (len) {
      var b = this.bytes.subarray(this.position, this.position + len);
      this.position += len;
      return b;
    },
      moveforward : function (len) {
          this.position += len;
      },

    peak : function (len) {
        var b = this.bytes.subarray(this.position, this.position + len);
        return b;
    },
    read_ascii : function (len) {
      return String.fromCharCode.apply(null, this.read(len));
    },
    read_text : function (len) {
      var b = this.read_ascii(len);
      return b;
    },
    read_int : function () {
      var b = this.read(4);
      return  b[0] << 0 |
              b[1] << 8  |
              b[2] << 16 |
              b[3] << 24;
    },
    peak_int : function () {
        var b = this.peak(4);
        return  b[0] << 0 |
            b[1] << 8  |
            b[2] << 16 |
            b[3] << 24;
    },
    read_int16 : function () {
      var b = this.read(2);
      return b[0] | b[1] << 8;
    }
  };

export function zip_GetFile(filerecord, bytes)
{
    let byteArray = new Uint8Array(bytes);
    let compresseddata = byteArray.slice(filerecord.startpos, filerecord.endpos);

    if(filerecord.method === 0)
    {
        return compresseddata;
    }

    var decompressedSize = filerecord.fileSize;
    var outputBuffer = new Uint8Array(decompressedSize);
    tinyinflate(compresseddata, outputBuffer);

    return outputBuffer
}

export function zip_Index(bytes) {

    var bin = new ByteReader(bytes);
    var sign_types = {
        file : 0x04034b50,
        header : 0x08074b50,
        directory : 0x02014b50,
        record : 0x06054b50,
    };
    var zip = new ZipData;
    var file;

    while (!bin.eos()) {

        var sign = bin.peak_int();
        if ( sign == sign_types.file )
        {
            bin.read_int();
            file = new File(bin);
            zip.files[file.name] = file;
        }
        else if ( sign == sign_types.header )
        {
            let offset = bin.position;

            bin.read_int();
            file.header = new File.Header(bin);
            file.header.offset = offset;

            file.size = file.header.size;
            file.fileSize = file.header.fileSize;

            file.startpos = offset - file.header.size;
            file.endpos = offset;
        }
        else if ( sign == sign_types.directory ) {
            bin.read_int();
            var directory = new Directory(bin);
            zip.directories[directory.name] = directory;
        }
        else if ( sign == sign_types.record ) {
            bin.read_int();
            var record = new Directory.Record(bin);
            zip.record = record;
        }
        else {
            bin.moveforward(1);
        }
    }

    return zip;
}

let zip_IndexAsync = co.wrap(function * (bytes, onprogress) {

    var bin = new ByteReader(bytes);
    var sign_types = {
        file : 0x04034b50,
        header : 0x08074b50,
        directory : 0x02014b50,
        record : 0x06054b50,
    };
    var zip = new ZipData;
    var file;
    let progress = 0;

    while (!bin.eos()) {

        var sign = bin.peak_int();

        if ( sign == sign_types.file )
        {
            bin.read_int();
            file = new File(bin);
            zip.files[file.name] = file;
        }
        else if ( sign == sign_types.header )
        {
            if(onprogress)
            {
                onprogress(file.name, progress += 0.05);
                yield Promise.waitForFrame();
            }

            let offset = bin.position;

            bin.read_int();
            file.header = new File.Header(bin);
            file.header.offset = offset;

            file.size = file.header.size;
            file.fileSize = file.header.fileSize;

            file.startpos = offset - file.header.size;
            file.endpos = offset;
        }
        else if ( sign == sign_types.directory ) {
            bin.read_int();
            var directory = new Directory(bin);
            zip.directories[directory.name] = directory;
        }
        else if ( sign == sign_types.record ) {
            bin.read_int();
            var record = new Directory.Record(bin);
            zip.record = record;
        }
        else {
            bin.moveforward(1);
        }
    }

    return zip;
});

export {zip_IndexAsync};
