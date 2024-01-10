(* Free Pascal Compiler version 3.2.2 *)
program babyheap;
var
   g_size : array[0..3] of integer;
   g_arr  : array[0..3] of array of integer;

(* Utility functions *)
function get_id(): integer;
var
   id : integer;
begin
   write('id: ');
   flush(output);
   read(id);
   if (id < 0) or (id > 3) then begin
      writeln('Invalid ID');
      halt(1);
   end;
   get_id := id;
end;

function get_index(id : integer): integer;
var
   index : integer;
begin
   write('index: ');
   flush(output);
   read(index);
   if (index < 0) or (index > g_size[id]) then begin
      writeln('Index out of range');
      halt(1);
   end;
   get_index := index;
end;

function get_size(): integer;
var
   size : integer;
begin
   write('size: ');
   flush(output);
   read(size);
   if (size <= 0) or (size >= 100) then begin
      writeln('Invalid size');
      halt(1);
   end;
   get_size := size;
end;

(* Core functions *)
procedure realloc();
var
   id : integer;
begin
   id := get_id();
   g_size[id] := get_size();
   setLength(g_arr[id], g_size[id]);
end;

procedure edit();
var
   id    : integer;
   index : integer;
begin
   id := get_id();
   index := get_index(id);
   write('value: ');
   flush(output);
   read(g_arr[id][index]);
end;

(* Entry point *)
var
   id     : integer;
   choice : integer;
begin
   for id := 0 to 3 do begin
      g_size[id] := 1;
      setLength(g_arr[id], g_size[id]);
   end;
   writeln('1. Realloc array');
   writeln('2. Edit array');
   repeat
      write('> ');
      flush(output);
      read(choice);
      case choice of
        1 : realloc();
        2 : edit();
      else
         writeln('Bye \(^o^)/');
         break;
      end;
   until false;
end.
