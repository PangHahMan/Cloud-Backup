document.getElementById("upload").addEventListener("click", function(){
    var fileInput = document.getElementById("file");
    var file = fileInput.files[0];
    var user_id = localStorage.getItem("user_id");

    var formData = new FormData();
    formData.append("file", file);

    fetch('/upload', {
        method: 'POST',
        headers: {
            'user_id': user_id
        },
        body: formData
    })
    .then(response => response.json())
    .then(result => {
        console.log('Success:', result);
        listFiles(); // Refresh the file list after upload
    })
    .catch(error => {
        console.error('Error:', error);
    });
});

function listFiles() {
    var user_id = localStorage.getItem("user_id");
    fetch('/listshow', {
        headers: {
            'user_id': user_id
        }
    })
    .then(response => response.json())
    .then(files => {
        var fileList = document.getElementById("fileList");
        fileList.innerHTML = ""; // Clear the file list first
        files.forEach(file => {
            var fileItem = document.createElement("p");
            fileItem.textContent = file.name;
            fileList.appendChild(fileItem);
        });
    });
}

// List files when the page loads
listFiles();
