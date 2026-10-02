// GLTFExporter (binary) reads Blobs through FileReader, which Node lacks.
if (!globalThis.FileReader) {
  globalThis.FileReader = class {
    _done(r) { this.result = r; this.onload?.({ target: this }); this.onloadend?.({ target: this }); }
    readAsArrayBuffer(b) { b.arrayBuffer().then(r => this._done(r)); }
    readAsDataURL(b) {
      b.arrayBuffer().then(r => this._done('data:' + (b.type || 'application/octet-stream') + ';base64,' + Buffer.from(r).toString('base64')));
    }
  };
}
