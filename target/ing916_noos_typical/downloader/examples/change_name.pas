const
  NAME_ADDR = $0;

procedure OnStartRun(const BatchCounter: Integer; var Abort: Boolean);
begin
  Print('OnStartRun %d', [BatchCounter]);
end;

procedure OnStartBin(const BatchCounter, BinIndex: Integer;
                     var Data: TBytes; var Abort: Boolean);
var
  N: string;
  I: Integer;
begin
  if BinIndex <> 2 then Exit;
  Print('size = %d', [Length(Data)]);
  N := Format('%0.4X', [BatchCounter]);
  for I := 1 to Length(N) do
      Data[BD_ADDR_ADDR + I - 1] := Ord(N[I]);
end;



