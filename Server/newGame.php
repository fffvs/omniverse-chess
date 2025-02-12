<?php
  //18 August 2020
  //18/8/20
  
  //Set the default timezone
  date_default_timezone_set("Australia/Melbourne");
  
  
  //$chessroot = "/tmp/4DChessV1.1";
  $chessroot = "/srv/store/4DChessV1.1";
  //If the game path doesn't exist, create it (this should only need to be executed once per server reboot)
  if(!file_exists($chessroot)){
    mkdir($chessroot);
    chmod($chessroot, 0777);
    //Write to the log file
    file_put_contents($chessroot."/log.txt","[".date("Y-m-d H:i:s",time())."] Created the ".$chessroot."/ directory".PHP_EOL,FILE_APPEND | LOCK_EX);
  }
  
  
  //Handle values supplied through GET/POST
  $gameid = $_REQUEST["id"];
  //Confirm that $gameid is valid (containing only alphanumeric characters)
  if(!isset($gameid) || $gameid == "" || strlen($gameid) != 8 || !ctype_alnum($gameid)){
    //Game ID not supplied or invalid, exit code 1
    exit("1");
  }
  
  if($gameid == "00000000"){
    //Reserved game ID, exit code 2
    exit("2");
  }
  
  $gamepath = $chessroot."/".$gameid;
  
  $layout = $_REQUEST["l"];
  //Set to the default layout if an invalid layout is specified
  if(!isset($layout) || $layout == "" || !($layout >= 0 && $layout <= 8)){
    $layout = 0;
  }
  
  $password = $_REQUEST["passw"];
  //Sanitise the string contained in $password
  if(!isset($password) || !ctype_alnum($password)){
    $password = null;
  }
  
  
  
  //Make sure a game with the specified ID doesn't already exist
  if(file_exists($gamepath)){
    //Game ID already in use, exit code 2
    exit("2");
  }
  
  //If all tests have been passed, create a new game (and fix permissions)
  if(mkdir($gamepath)){
    chmod($gamepath, 0777);
  }else{
    //Write error, exit code 3
    exit("3");
  }
  
  //If a password is supplied, add it to a file for later use
  if(isset($password)){
    $passwfile = fopen($gamepath."/passw","w");
    fwrite($passwfile,$password.PHP_EOL);
    fclose($passwfile);
    chmod($gamepath."/passw", 0777);
  }
  
  //Create the "layout" file, which stores the starting layout of the game
  $layoutfile = fopen($gamepath."/layout","w");
  fwrite($layoutfile,$layout.PHP_EOL);
  fclose($layoutfile);
  chmod($gamepath."/layout", 0777);
  
  //Create the "moves" file, which stores the amount of moves (total and made by each player)
  $movefile = fopen($gamepath."/moves","w");
  ///fwrite($movefile,"-".PHP_EOL."0".PHP_EOL."0".PHP_EOL);
  fwrite($movefile,"[-1,0,0]");
  fclose($movefile);
  chmod($gamepath."/moves", 0777);
  
  //Create the "gamestate" file, which stores the entire current game as JSON
  if(touch($gamepath."/gamestate.json")){
    chmod($gamepath."/gamestate.json", 0777);
  }else{
    exit("3");
  }
  
  //Create the "chat" file
  if(touch($gamepath."/chat")){
    chmod($gamepath."/chat", 0777);
  }else{
    //Write error, exit code 3
    exit("3");
  }
  
  //Write to the log file
  file_put_contents($chessroot."/log.txt","[".date("Y-m-d H:i:s",time())."] Created a new game with ID: ".$gameid.PHP_EOL,FILE_APPEND | LOCK_EX);
  
  //Everything was successful, exit with code 0
  exit("0");
?>
