var media = new function() {

	// This function is to create the media list.
	// @return {[type]} [description]
	// var medialist;
	this.playlist = new function() {
		this.listNum = null;
		this.listName = null;

		this.play = function() {


		}

		this.create = function(listNum, listName, imgPath) {
			// alert('hi/');
			this.listNum = listNum;
			this.listName = listName;

			li = document.createElement('li');
			li.className = "store";
			li.setAttribute("value", this.listNum);
			li.setAttribute("name", this.listName);

			var a = document.createElement('a');
			a.className = "noeffect";

			var img = document.createElement('img');
			img.setAttribute("src", imgPath);
			img.setAttribute("alt", "Radio Station Image");

			var sp1 = document.createElement('span');
			sp1.className = "name";
			sp1.innerHTML = listName;

			var sp2 = document.createElement('span');
			sp2.className = "arrow";

			li.appendChild(a);
			li.appendChild(img);
			li.appendChild(sp1);
			li.appendChild(sp2);
			return li;
		}
	}

};

// var media=(function(){

// 	function createMediaList(){
// 		alert('hi/');
// 	}

// 	return {
// 		createMediaList:createMediaList
// 	};
// })();
