import {Component, OnInit, Input, Output, EventEmitter} from '@angular/core';
import {BackendService} from '../../services/backend.service';
import {VariablesService} from '../../services/variables.service';

@Component({
  selector: 'app-staking-switch',
  templateUrl: './staking-switch.component.html',
  styleUrls: ['./staking-switch.component.scss']
})
export class StakingSwitchComponent implements OnInit {

  @Input() wallet_id: boolean;
  @Input() staking: boolean;
  @Output() stakingChange = new EventEmitter<boolean>();

  constructor(private backend: BackendService, private variablesService: VariablesService) {}

  ngOnInit() {}

  toggleStaking() {
    const wallet = this.variablesService.getWallet(this.wallet_id);
    if (wallet && wallet.loaded) {
      this.stakingChange.emit(!this.staking);
      if (!this.staking) {
        this.backend.startPosMining(this.wallet_id);
      } else {
        this.backend.stopPosMining(this.wallet_id);
      }
    }
  }
}
