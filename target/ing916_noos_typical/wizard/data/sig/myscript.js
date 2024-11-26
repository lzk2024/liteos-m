window.onload= function(){
    var lst = document.getElementsByTagName("a");
    var base = "https://www.bluetooth.com/wp-content/uploads/Sitecore-Media-Library/Gatt/Xml/";
    if (lst.length > 0)
    {
        var a = lst[0];
        var url = a.getAttribute("href");
        if (url.indexOf('service') >= 0)
            a.setAttribute("href", base + "Services/" + url + ".xml");
        else
            a.setAttribute("href",  base + "Characteristics/" + url + ".xml");
    }

    for (var i = 1; i < lst.length; i++)
    {
        var a = lst[i];
        var url = a.getAttribute("href");
        a.setAttribute("href", url + ".html");
    }
};
