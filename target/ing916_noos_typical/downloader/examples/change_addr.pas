// Two procedures are required to be provided:
// * OnStartRun, which gets called when each round of downloading starts
// * OnStartBin, which gets called when a binary starts downloading

// we can use constants
const
  BD_ADDR_ADDR = $1;

procedure OnStartRun(const BatchCounter: Integer; var Abort: Boolean);
begin
  // Use *Print* for logging and debugging
  Print('OnStartRun %d', [BatchCounter]);
  // we can abort downloading by assigning True to *Abort*
  // Abort := True;
end;

procedure OnStartBin(const BatchCounter, BinIndex: Integer;
                     var Data: TBytes; var Abort: Boolean);
begin
  // Note that BinIndex counts from 1 (not 0), just as shown on GUI
  if BinIndex <> 2 then Exit;
  Print('size = %d', [Length(Data)]);
  // We can modify binary data before it is downloaded into flash
  Data[BD_ADDR_ADDR + 0] := BatchCounter and $FF;
  Data[BD_ADDR_ADDR + 1] := (BatchCounter shr 8) and $FF;
  Data[BD_ADDR_ADDR + 2] := (BatchCounter shr 8) and $FF;
end;


