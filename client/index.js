'use strict';

const sendRequest = async (method, body = null) => {
    const response = fetch('http://localhost:3018', {
        method: method,
        mode: 'cors',
        cache: "no-cache",
        credentials: "omit",
        headers: {
            "Content-Type": "text/plain"
        },
        body: body
    });
    return await response.then(value => value.text())
        .then(text => {
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
        sendRequest("POST", "\b");
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
    }
    else {
        sendRequest("POST", e.key);
    }
};

document.addEventListener("keydown", keyDownHandler);

const reqMonitorBuffer = async () => {
    const body = await sendRequest("GET");
    let elem = document.getElementById("monitor_buffer");
    elem.innerHTML = body;
};

setInterval(reqMonitorBuffer, 1000);
// reqMonitorBuffer();