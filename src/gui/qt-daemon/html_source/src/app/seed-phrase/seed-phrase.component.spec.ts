import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { SeedPhraseComponent } from './seed-phrase.component';

describe('SeedPhraseComponent', () => {
  let component: SeedPhraseComponent;
  let fixture: ComponentFixture<SeedPhraseComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ SeedPhraseComponent ]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(SeedPhraseComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
