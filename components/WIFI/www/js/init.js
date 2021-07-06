var cfg = {};
$(document).ready(function(){
	$("#preloader-container").fadeIn(500);

	$('ul.tabs').tabs();

	$('.button-collapse').sideNav({
		closeOnClick: true,
      	draggable: true
    });

	$('select').material_select();

	$("#cmdShowPWD").change(function(){
		var isChecked = $(this).is(":checked");
		if(isChecked){
			$("#staPWD").attr("type", "text");
		}
		else{
			$("#staPWD").attr("type", "password");	
		}
	});

	$("#cmdShowMqtt").change(function(){
		var isChecked = $(this).is(":checked");
		if(isChecked){
			$("#mqtt-setup-container").fadeIn(500);
		}
		else{
			$("#mqtt-setup-container").fadeOut(500);
		}
	});

	$('#nav-desktop').tabs({
		onShow:  function(e){
			var el =  $(this);
			$('#nav-desktop li').removeClass("active");
			$('#nav-mobile li').removeClass("selected");
			$('#nav-mobile li')
			el.parent().addClass("active");
		}
	});

	$('#nav-desktop').tabs({
		onShow:  function(e){
			var el =  $(this);
			$('#nav-desktop li').removeClass("active");
			$('#nav-mobile li').removeClass("selected");
			$('#nav-mobile a').removeClass("active");
			$('#nav-mobile li a[href="' + el.attr("href") + '"]').addClass("active").parent().addClass("selected");
			el.parent().addClass("active");
		}
	});	

	$("#nav-mobile a").click(function(e){
		var el = $(this);
		$('#nav-desktop a[href="' + el.attr("href") + '"]').trigger("click");
	});

	$("#cmdScanWiFi").click(function(){
		scanWiFi();
	});

	$("#cmdConnectWiFi").click(function(){
		$(this).attr("disabled");
		connectWiFi();
	});

	$("#cmdSetMqtt").click(function(){
		setupMqtt();
	});

	$("#cmdResetDevice").click(function(){
		rebootDevice();
	});

	$("#cmdLocalUpdateFirmware").click(function(){
		localUpdateFirmware();
	});

	$("#cmdFactoryReset").click(function(){
		factoryReset();
	});

	loadData();

	$("#selLanguage").change(function(){
		var lang = $(this).val();
		var language = new Language();
		language.setLanguage(lang);
		language.init();
	}).trigger("change");
});

function getDate(TS, format){
	var date = new Date(TS);
	var dd = date.getDate();
	var mm = date.getMonth() + 1;
	var yyyy = date.getFullYear();
	if(format == "DD-MM-YYYY"){
		return (dd >= 10 ? dd : '0' + dd) + "-" + (mm >= 10 ? mm : '0' + mm) + "-" + yyyy;
	}
	if(format == "DD/MM/YYYY"){
		return (dd >= 10 ? dd : '0' + dd) + "/" + (mm >= 10 ? mm : '0' + mm) + "/" + yyyy;
	}
	if(format == "YYYY-MM-DD"){
		return yyyy + "-" + (mm >= 10 ? mm : '0' + mm) + "-" + (dd >= 10 ? dd : '0' + dd);
	}
	if(format == "YYYY/MM/DD"){
		return yyyy + "/" + (mm >= 10 ? mm : '0' + mm) + "/" + (dd >= 10 ? dd : '0' + dd);
	}
}

function loadData(){
	$.get("/api/v1/get_data", function(json){
		scanWiFi();
		cfg = json;

		$("#mqttServer").val(cfg["mqtt_server"]);
		$("#mqttPort").val(cfg["mqtt_port"]);
		$("#mqttUser").val(cfg["mqtt_username"]);
		$("#mqttPwd").val(cfg["mqtt_password"]);
		
		$("#device_name").html(cfg["product_name"]);
		$("#device_serial").html(cfg["product_serial"]);
		$("#device_company").html(cfg["company_name"]);
		$("#device_release_date").html(getDate(cfg["product_release_date"], "DD-MM-YYYY"));

		$("#device_hardware_version").html(cfg["hw_version"]);
		$("#device_software_version").html(cfg["sw_version"]);
		$("#device_software_date").html(getDate(cfg["sw_date"], "DD-MM-YYYY"));
	});
};

function scanWiFi(){
	$("#cmdScanWiFi").attr("disabled", "disabled");
	$.get("/api/v1/scan_wifi", function(json){
		var sel = $("#staSSID");
		var opt = "<option value='' disabled selected>" + LANG["actions"]["chooseWiFi"] + "</option>";
		sel.html(opt);

		for(var i = 0; i<json.length; i++){
			var selected = (json[i]["name"] == cfg["sta_ssid"] ? "selected" : "");
			var opt = "<option value='" + json[i]["name"] + "' " + selected + ">" + json[i]["name"] + (json[i]["encrypted"] == true ? "<i class='material-icons md-18 icon-lock right'></i>" : "") + "</option>";
			sel.append(opt); 
		}
		sel.material_select();
		if(sel.val() != ""){
			$("#staPWD").val(cfg["sta_pwd"]).focus().blur();
		}

		sel.change(function(){
			if(sel.val() != cfg["sta_ssid"]){
				$("#staPWD").val(cfg["sta_pwd"]).focus().blur();
			}
			else{
				$("#staPWD").val("").blur();
			}
		});

		$("#cmdScanWiFi").removeAttr("disabled");
		$("#preloader-container").fadeOut(500);
	});
}

function connectWiFi(){
	var ssid = $("#staSSID").val();
	var pwd = $("#staPWD").val();
	if(ssid == ""){
		var $toastContent = $('<span class="">' + LANG["actions"]["emptySSID"] + '</span>');
  		Materialize.toast($toastContent, 5000);
  		$("#staSSID").focus();
		return;
	}
	$("#cmdConnectWiFi").attr("disabled", "disabled");
	$.post("/api/v1/connect_wifi", {ssid: ssid, pwd: pwd}, function(json){
		if(json["success"])
		{
			var $toastContent = $('<span class="">' + LANG["actions"]["connectSuccessfully"] + '</span>');
  			Materialize.toast($toastContent, 5000);
  			$("#wifiActionPage").fadeOut(500);
			$("#rebootActionPage").fadeIn(500);
		}

		else{
			var $toastContent = $('<span class="">' + LANG["actions"]["connectFailed"] + '</span>');
  			Materialize.toast($toastContent, 5000);
		}
		$("#cmdConnectWiFi").removeAttr("disabled");
		
	});
}

function setupMqtt(){
	var server = $("#mqttServer").val();
	var port = $("#mqttPort").val();
	var user = $("#mqttUser").val();
	var password = $("#mqttPwd").val();

	if(server == ""){
		var $toastContent = $('<span class="">' + LANG["actions"]["emptyMqttServer"] + '</span>');
  		Materialize.toast($toastContent, 5000);
  		$("#mqttServer").focus();
		return;
	}

	if(port == ""){
		var $toastContent = $('<span class="">' + LANG["actions"]["emptyMqttPort"] + '</span>');
  		Materialize.toast($toastContent, 5000);
  		$("#mqttPort").focus();
		return;
	}

	if(user == ""){
		var $toastContent = $('<span class="">' + LANG["actions"]["emptyMqttUser"] + '</span>');
  		Materialize.toast($toastContent, 5000);
  		$("#mqttUser").focus();
		return;
	}

	if(password == ""){
		var $toastContent = $('<span class="">' + LANG["actions"]["emptyMqttPwd"] + '</span>');
  		Materialize.toast($toastContent, 5000);
  		$("#mqttPwd").focus();
		return;
	}

	$("#cmdSetMqtt").attr("disabled", "disabled");
	
	var params = {
		mqtt_server: server,
		mqtt_port: port,
		mqtt_username: user,
		mqtt_password: password
	}

	$.post("/api/v1/setup_mqtt", params, function(json){
		if(json["success"])
		{
			var $toastContent = $('<span class="">' + LANG["actions"]["setMqttSuccess"] + '</span>');
  			Materialize.toast($toastContent, 5000);
		}

		else{
			var $toastContent = $('<span class="">' + LANG["actions"]["setMqttFailed"] + '</span>');
  			Materialize.toast($toastContent, 5000);
		}
		$("#cmdSetMqtt").removeAttr("disabled");
	});
}

function rebootDevice(){
	$("#cmdResetDevice").attr("disabled", "disabled");
	$.post("/api/v1/reboot_device", function(json){
		if(json["success"]){
			var $toastContent = $('<span class="">' + LANG["actions"]["deviceRestarted"] + '</span>');
  			Materialize.toast($toastContent, 10000);
  			window.setTimeout('location.reload()', 10000); //Reloads after three seconds
		}	
		else{
			$("#cmdResetDevice").removeAttr("disabled");
		}	
	});
};

function localUpdateFirmware(){
	var adminPWD = $("#adminPWD").val();
	if(adminPWD == ""){
		var $toastContent = $('<span class="">' + LANG["actions"]["emptyPassword"] + '</span>');
		Materialize.toast($toastContent, 5000);
		return;
	}

	if(adminPWD != "support@agrhub.com"){
		var $toastContent = $('<span class="">' + LANG["actions"]["invalidPassword"] + '</span>');
		Materialize.toast($toastContent, 5000);
		return;
	}

	var file = $("#firmwareFile")[0].files;
	if (!file.length < 0 ){
		var $toastContent = $('<span class="">' + LANG["actions"]["emptyFirmware"] + '</span>');
		Materialize.toast($toastContent, 5000);
		return;
	}

	$("#cmdLocalUpdateFirmware").attr("disabled", "disabled");

	var data = new FormData(document.getElementById("localUpdateForm")); // your form ID
	
	$.ajax({
	    url: "/ota_update?admin_password=" + adminPWD,
	    type: "POST",
	    data: data, 
	    enctype: 'multipart/form-data',
	    processData: false,
	    contentType: false,  
	    dataType: 'json',
	    success: function(response) {        
	        if(response["success"]){
	        	var $toastContent = $('<span class="">' + LANG["actions"]["updateSuccessfully"] + '</span>');
				Materialize.toast($toastContent, 10000);
	        }
	        else{
	        	var $toastContent = $('<span class="">' + LANG["actions"]["updateFailed"] + '</span>');
				Materialize.toast($toastContent, 5000);
	        }
	        $("#cmdLocalUpdateFirmware").removeAttr("disabled");
	    },
	    fail : function(){
	    	var $toastContent = $('<span class="">' + LANG["actions"]["updateFailed"] + '</span>');
			Materialize.toast($toastContent, 5000);
			$("#cmdLocalUpdateFirmware").removeAttr("disabled");
	    }
	}); 
};

function compareSwVersion(oldVersion, newVersion){
	var olds = oldVersion.split(".");
	var news = newVersion.split(".");
	if(olds.length != 3 || news.length != 3){
		return false;
	}
	var result = false;
	for(var i = 0; i < 3; i++){
		if(parseInt(news[i]) > parseInt(olds[i])){
			result = true;
			break;
		}
	}

	return result;
}

function factoryReset(){
	$("#cmdFactoryReset").attr("disabled", "disabled");
	$.post("/api/v1/factory_reset?", {}, function(json){
		if(json["success"]){
			var $toastContent = $('<span class="">' + LANG["actions"]["factoryResetSuccessfully"] + '</span>');
			Materialize.toast($toastContent, 10000);
		}
		else{
			var $toastContent = $('<span class="">' + LANG["actions"]["factoryResetFailed"] + '</span>');
			Materialize.toast($toastContent, 5000);
			$("#cmdFactoryReset").removeAttr("disabled");
		}
	});
}
