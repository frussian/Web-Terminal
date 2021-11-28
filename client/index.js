'use strict';

const sendRequest = async (method, body = null) => {
    const response = fetch('http://localhost:3018', {
        method: method,
        mode: 'cors',
        cache: "no-cache",
        credentials: "omit",
        headers: {
            "Content-Type": "text/plain;charset=UTF-8"
        },
        body: body
    });
    return await response.then(value => value.text())
        .then(text => {
            if (method === "GET") {
                console.log(text);
                console.log(toUTF8Array(text));
            }
            return text;
        })
        .catch(err => console.log(err));
};

const keyDownHandler = (e) => {
    console.log(e.key, e.ctrlKey, e.code);
    e.preventDefault();
    const alphanumeric = /^[\p{sc=Latn}\p{sc=Cyrillic}\p{Nd}]+$/u;

    if (e.key === "Shift" || e.key === "Alt" ||
        e.key === "Control" || e.key === "CapsLock" ||
        e.key === "Insert") {
        return;
    }

    if (e.ctrlKey && e.key >= 'a' && e.key <= 'z') {
        let code = e.key.charCodeAt(0) - 0x20 - 0x40;
        let char = String.fromCharCode(code);
        sendRequest("POST", char);
    } else if (e.key === "Backspace") {
        sendRequest("POST", "\x7f");
    } else if (e.key === "Tab") {
        sendRequest("POST", "\t");
    } else if (e.key === "Enter") {
        sendRequest("POST", "newline");
    } else if (e.key === "Escape") {
        sendRequest("POST", "\x1b");
    } else if (e.key === "Home") {
        sendRequest("POST", "\x1b[1~");
    } else if (e.key === "ArrowLeft") {
        sendRequest("POST", "\x1b[D");
    } else if (e.key === "ArrowRight") {
        sendRequest("POST", "\x1b[C");
    } else if (e.key === "ArrowUp") {
        sendRequest("POST", "\x1b[A");
    } else if (e.key === "ArrowDown") {
        sendRequest("POST", "\x1b[B");
    }
    else {
        sendRequest("POST", e.key);
    }
};

document.addEventListener("keydown", keyDownHandler);

const reqMonitorBuffer = async () => {
    const body = await sendRequest("GET");
    let elem = document.getElementById("monitor_buffer");
    if (body === "no changes") return;
    elem.innerHTML = body;
};

reqMonitorBuffer();
let intervalID = setInterval(reqMonitorBuffer, 1000);

function toUTF8Array(str) {
    var utf8 = [];
    for (var i=0; i < str.length; i++) {
        var charcode = str.charCodeAt(i);
        if (charcode < 0x80) utf8.push(charcode);
        else if (charcode < 0x800) {
            utf8.push(0xc0 | (charcode >> 6),
                0x80 | (charcode & 0x3f));
        }
        else if (charcode < 0xd800 || charcode >= 0xe000) {
            utf8.push(0xe0 | (charcode >> 12),
                0x80 | ((charcode>>6) & 0x3f),
                0x80 | (charcode & 0x3f));
        }
        // surrogate pair
        else {
            i++;
            // UTF-16 encodes 0x10000-0x10FFFF by
            // subtracting 0x10000 and splitting the
            // 20 bits of 0x0-0xFFFFF into two halves
            charcode = 0x10000 + (((charcode & 0x3ff)<<10)
                | (str.charCodeAt(i) & 0x3ff));
            utf8.push(0xf0 | (charcode >>18),
                0x80 | ((charcode>>12) & 0x3f),
                0x80 | ((charcode>>6) & 0x3f),
                0x80 | (charcode & 0x3f));
        }
    }
    return utf8;
}