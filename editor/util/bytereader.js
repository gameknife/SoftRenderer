export default class ByteReader
{
    constructor(bytes) {
        this.bytes = new Uint8Array( bytes );
        this.length = this.bytes.length;
        this.position = 0;
    }

    eos() {
    return this.position >= this.length;
    };
    slice(start, end) {
    var b = this.bytes.subarray(start, end);
    return b;
};
    read(len) {
    var b = this.bytes.subarray(this.position, this.position + len);
    this.position += len;
    return b;
};
    peak(len) {
    var b = this.bytes.subarray(this.position, this.position + len);
    return b;
};
    read_ascii(len) {
    return String.fromCharCode.apply(null, this.read(len));
};
    read_text(len) {
    var b = this.read_ascii(len);
    return b.sjis2utf16 ? b.sjis2utf16() : b;
};
    read_int() {
    var b = this.read(4);
    return  b[0] << 0 |
        b[1] << 8  |
        b[2] << 16 |
        b[3] << 24;
};
    peak_int() {
    var b = this.peak(4);
    return  b[0] << 0 |
        b[1] << 8  |
        b[2] << 16 |
        b[3] << 24;
};
    read_int16() {
    var b = this.read(2);
    return b[0] | b[1] << 8;
}
}
