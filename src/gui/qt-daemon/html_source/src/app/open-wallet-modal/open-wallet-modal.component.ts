import {Component, OnInit, Input, Output, EventEmitter} from '@angular/core';
import {VariablesService} from "../_helpers/services/variables.service";

@Component({
  selector: 'app-open-wallet-modal',
  templateUrl: './open-wallet-modal.component.html',
  styleUrls: ['./open-wallet-modal.component.scss']
})
export class OpenWalletModalComponent implements OnInit {

  @Input() path;
  @Output() onOpen = new EventEmitter<boolean>();
  @Output() onSkip = new EventEmitter<boolean>();

  constructor(public variablesService: VariablesService) {}

  ngOnInit() {}

  open() {
    this.onOpen.emit(true);
  }

  skip() {
    this.onSkip.emit(true);
  }

}
